/*
 * Copyright (c) 2011-2012 - Mauro Carvalho Chehab <mchehab@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "descriptors.h"
#include "dvb-fe.h"
#include "dvb-scan.h"
#include "parse_string.h"
#include "dvb-frontend.h"
#include "dvb-v5-std.h"
#include "dvb-log.h"

#include "descriptors/pat.h"
#include "descriptors/pmt.h"
#include "descriptors/nit.h"
#include "descriptors/sdt.h"
#include "descriptors/eit.h"
#include "descriptors/vct.h"
#include "descriptors/desc_language.h"
#include "descriptors/desc_network_name.h"
#include "descriptors/desc_cable_delivery.h"
#include "descriptors/desc_sat.h"
#include "descriptors/desc_terrestrial_delivery.h"
#include "descriptors/desc_service.h"
#include "descriptors/desc_service_list.h"
#include "descriptors/desc_frequency_list.h"
#include "descriptors/desc_event_short.h"
#include "descriptors/desc_event_extended.h"
#include "descriptors/desc_atsc_service_location.h"
#include "descriptors/desc_hierarchy.h"
#include "descriptors/desc_extension.h"

ssize_t dvb_desc_init(const uint8_t *buf, struct dvb_desc *desc)
{
	desc->type   = buf[0];
	desc->length = buf[1];
	desc->next   = NULL;
	return 2;
}

void dvb_desc_default_init(struct dvb_v5_fe_parms *parms, const uint8_t *buf, struct dvb_desc *desc)
{
	memcpy(desc->data, buf, desc->length);
}

void dvb_desc_default_print(struct dvb_v5_fe_parms *parms, const struct dvb_desc *desc)
{
	dvb_log("|                   %s (0x%02x)", dvb_descriptors[desc->type].name, desc->type);
	hexdump(parms, "|                       ", desc->data, desc->length);
}

const struct dvb_table_init dvb_table_initializers[] = {
	[DVB_TABLE_PAT] = { dvb_table_pat_init },
	[DVB_TABLE_PMT] = { dvb_table_pmt_init },
	[DVB_TABLE_NIT] = { dvb_table_nit_init },
	[DVB_TABLE_SDT] = { dvb_table_sdt_init },
	[DVB_TABLE_EIT] = { dvb_table_eit_init },
	[DVB_TABLE_TVCT] = { dvb_table_vct_init },
	[DVB_TABLE_CVCT] = { dvb_table_vct_init },
	[DVB_TABLE_EIT_SCHEDULE] = { dvb_table_eit_init },
};

char *default_charset = "iso-8859-1";
char *output_charset = "utf-8";

void dvb_parse_descriptors(struct dvb_v5_fe_parms *parms, const uint8_t *buf, uint16_t section_length, struct dvb_desc **head_desc)
{
	const uint8_t *ptr = buf;
	struct dvb_desc *current = NULL;
	struct dvb_desc *last = NULL;
	while (ptr < buf + section_length) {
		int desc_type = ptr[0];
		int desc_len  = ptr[1];
		size_t size;

		dvb_desc_init_func init = dvb_descriptors[desc_type].init;
		if (!init) {
			init = dvb_desc_default_init;
			size = sizeof(struct dvb_desc) + desc_len;
		} else {
			size = dvb_descriptors[desc_type].size;
		}
		if (!size) {
			dvb_logerr("descriptor type %d has no size defined", current->type);
			size = 4096;
		}
		current = malloc(size);
		if (!current)
			dvb_perror("Out of memory");
		ptr += dvb_desc_init(ptr, current); /* the standard header was read */
		if (ptr >=  buf + section_length) {
			dvb_logerr("descriptor is truncated");
			return;
		}
		init(parms, ptr, current);
		if(!*head_desc)
			*head_desc = current;
		if (last)
			last->next = current;
		last = current;
		ptr += current->length;     /* standard descriptor header plus descriptor length */
	}
}

void dvb_print_descriptors(struct dvb_v5_fe_parms *parms, struct dvb_desc *desc)
{
	while (desc) {
		dvb_desc_print_func print = dvb_descriptors[desc->type].print;
		if (!print)
			print = dvb_desc_default_print;
		print(parms, desc);
		desc = desc->next;
	}
}

void dvb_free_descriptors(struct dvb_desc **list)
{
	struct dvb_desc *desc = *list;
	while (desc) {
		struct dvb_desc *tmp = desc;
		desc = desc->next;
		if (dvb_descriptors[tmp->type].free)
			dvb_descriptors[tmp->type].free(tmp);
		else
			free(tmp);
	}
	*list = NULL;
}

const struct dvb_descriptor dvb_descriptors[] = {
	[0 ...255 ] = {
		.name  = "Unknown descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[video_stream_descriptor] = {
		.name  = "video_stream_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[audio_stream_descriptor] = {
		.name  = "audio_stream_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[hierarchy_descriptor] = {
		.name  = "hierarchy_descriptor",
		.init  = dvb_desc_hierarchy_init,
		.print = dvb_desc_hierarchy_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_hierarchy),
	},
	[registration_descriptor] = {
		.name  = "registration_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[ds_alignment_descriptor] = {
		.name  = "ds_alignment_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[target_background_grid_descriptor] = {
		.name  = "target_background_grid_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[video_window_descriptor] = {
		.name  = "video_window_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[conditional_access_descriptor] = {
		.name  = "conditional_access_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[iso639_language_descriptor] = {
		.name  = "iso639_language_descriptor",
		.init  = dvb_desc_language_init,
		.print = dvb_desc_language_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_language),
	},
	[system_clock_descriptor] = {
		.name  = "system_clock_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[multiplex_buffer_utilization_descriptor] = {
		.name  = "multiplex_buffer_utilization_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[copyright_descriptor] = {
		.name  = "copyright_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[maximum_bitrate_descriptor] = {
		.name  = "maximum_bitrate_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[private_data_indicator_descriptor] = {
		.name  = "private_data_indicator_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[smoothing_buffer_descriptor] = {
		.name  = "smoothing_buffer_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[std_descriptor] = {
		.name  = "std_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[ibp_descriptor] = {
		.name  = "ibp_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[mpeg4_video_descriptor] = {
		.name  = "mpeg4_video_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[mpeg4_audio_descriptor] = {
		.name  = "mpeg4_audio_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[iod_descriptor] = {
		.name  = "iod_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[sl_descriptor] = {
		.name  = "sl_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[fmc_descriptor] = {
		.name  = "fmc_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[external_es_id_descriptor] = {
		.name  = "external_es_id_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[muxcode_descriptor] = {
		.name  = "muxcode_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[fmxbuffersize_descriptor] = {
		.name  = "fmxbuffersize_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[multiplexbuffer_descriptor] = {
		.name  = "multiplexbuffer_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[content_labeling_descriptor] = {
		.name  = "content_labeling_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[metadata_pointer_descriptor] = {
		.name  = "metadata_pointer_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[metadata_descriptor] = {
		.name  = "metadata_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[metadata_std_descriptor] = {
		.name  = "metadata_std_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[AVC_video_descriptor] = {
		.name  = "AVC_video_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[ipmp_descriptor] = {
		.name  = "ipmp_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[AVC_timing_and_HRD_descriptor] = {
		.name  = "AVC_timing_and_HRD_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[mpeg2_aac_audio_descriptor] = {
		.name  = "mpeg2_aac_audio_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[flexmux_timing_descriptor] = {
		.name  = "flexmux_timing_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[network_name_descriptor] = {
		.name  = "network_name_descriptor",
		.init  = dvb_desc_network_name_init,
		.print = dvb_desc_network_name_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_network_name),
	},
	[service_list_descriptor] = {
		.name  = "service_list_descriptor",
		.init  = dvb_desc_service_list_init,
		.print = dvb_desc_service_list_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_service_list),
	},
	[stuffing_descriptor] = {
		.name  = "stuffing_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[satellite_delivery_system_descriptor] = {
		.name  = "satellite_delivery_system_descriptor",
		.init  = dvb_desc_sat_init,
		.print = dvb_desc_sat_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_sat),
	},
	[cable_delivery_system_descriptor] = {
		.name  = "cable_delivery_system_descriptor",
		.init  = dvb_desc_cable_delivery_init,
		.print = dvb_desc_cable_delivery_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_cable_delivery),
	},
	[VBI_data_descriptor] = {
		.name  = "VBI_data_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[VBI_teletext_descriptor] = {
		.name  = "VBI_teletext_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[bouquet_name_descriptor] = {
		.name  = "bouquet_name_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[service_descriptor] = {
		.name  = "service_descriptor",
		.init  = dvb_desc_service_init,
		.print = dvb_desc_service_print,
		.free  = dvb_desc_service_free,
		.size  = sizeof(struct dvb_desc_service),
	},
	[country_availability_descriptor] = {
		.name  = "country_availability_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[linkage_descriptor] = {
		.name  = "linkage_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[NVOD_reference_descriptor] = {
		.name  = "NVOD_reference_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[time_shifted_service_descriptor] = {
		.name  = "time_shifted_service_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[short_event_descriptor] = {
		.name  = "short_event_descriptor",
		.init  = dvb_desc_event_short_init,
		.print = dvb_desc_event_short_print,
		.free  = dvb_desc_event_short_free,
		.size  = sizeof(struct dvb_desc_event_short),
	},
	[extended_event_descriptor] = {
		.name  = "extended_event_descriptor",
		.init  = dvb_desc_event_extended_init,
		.print = dvb_desc_event_extended_print,
		.free  = dvb_desc_event_extended_free,
		.size  = sizeof(struct dvb_desc_event_extended),
	},
	[time_shifted_event_descriptor] = {
		.name  = "time_shifted_event_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[component_descriptor] = {
		.name  = "component_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[mosaic_descriptor] = {
		.name  = "mosaic_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[stream_identifier_descriptor] = {
		.name  = "stream_identifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[CA_identifier_descriptor] = {
		.name  = "CA_identifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[content_descriptor] = {
		.name  = "content_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[parental_rating_descriptor] = {
		.name  = "parental_rating_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[teletext_descriptor] = {
		.name  = "teletext_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[telephone_descriptor] = {
		.name  = "telephone_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[local_time_offset_descriptor] = {
		.name  = "local_time_offset_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[subtitling_descriptor] = {
		.name  = "subtitling_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[terrestrial_delivery_system_descriptor] = {
		.name  = "terrestrial_delivery_system_descriptor",
		.init  = dvb_desc_terrestrial_delivery_init,
		.print = dvb_desc_terrestrial_delivery_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_terrestrial_delivery),
	},
	[multilingual_network_name_descriptor] = {
		.name  = "multilingual_network_name_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[multilingual_bouquet_name_descriptor] = {
		.name  = "multilingual_bouquet_name_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[multilingual_service_name_descriptor] = {
		.name  = "multilingual_service_name_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[multilingual_component_descriptor] = {
		.name  = "multilingual_component_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[private_data_specifier_descriptor] = {
		.name  = "private_data_specifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[service_move_descriptor] = {
		.name  = "service_move_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[short_smoothing_buffer_descriptor] = {
		.name  = "short_smoothing_buffer_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[frequency_list_descriptor] = {
		.name  = "frequency_list_descriptor",
		.init  = dvb_desc_frequency_list_init,
		.print = dvb_desc_frequency_list_print,
		.free  = NULL,
		.size  = sizeof(struct dvb_desc_frequency_list),
	},
	[partial_transport_stream_descriptor] = {
		.name  = "partial_transport_stream_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[data_broadcast_descriptor] = {
		.name  = "data_broadcast_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[scrambling_descriptor] = {
		.name  = "scrambling_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[data_broadcast_id_descriptor] = {
		.name  = "data_broadcast_id_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[transport_stream_descriptor] = {
		.name  = "transport_stream_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[DSNG_descriptor] = {
		.name  = "DSNG_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[PDC_descriptor] = {
		.name  = "PDC_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[AC_3_descriptor] = {
		.name  = "AC_3_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[ancillary_data_descriptor] = {
		.name  = "ancillary_data_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[cell_list_descriptor] = {
		.name  = "cell_list_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[cell_frequency_link_descriptor] = {
		.name  = "cell_frequency_link_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[announcement_support_descriptor] = {
		.name  = "announcement_support_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[application_signalling_descriptor] = {
		.name  = "application_signalling_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[adaptation_field_data_descriptor] = {
		.name  = "adaptation_field_data_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[service_identifier_descriptor] = {
		.name  = "service_identifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[service_availability_descriptor] = {
		.name  = "service_availability_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[default_authority_descriptor] = {
		.name  = "default_authority_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[related_content_descriptor] = {
		.name  = "related_content_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[TVA_id_descriptor] = {
		.name  = "TVA_id_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[content_identifier_descriptor] = {
		.name  = "content_identifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[time_slice_fec_identifier_descriptor] = {
		.name  = "time_slice_fec_identifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[ECM_repetition_rate_descriptor] = {
		.name  = "ECM_repetition_rate_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[S2_satellite_delivery_system_descriptor] = {
		.name  = "S2_satellite_delivery_system_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[enhanced_AC_3_descriptor] = {
		.name  = "enhanced_AC_3_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[DTS_descriptor] = {
		.name  = "DTS_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[AAC_descriptor] = {
		.name  = "AAC_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[XAIT_location_descriptor] = {
		.name  = "XAIT_location_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[FTA_content_management_descriptor] = {
		.name  = "FTA_content_management_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[extension_descriptor] = {
		.name  = "extension_descriptor",
		.init  = extension_descriptor_init,
		.print = NULL,
		.free  = NULL,
		.size  = sizeof(struct dvb_extension_descriptor),
	},

	[CUE_identifier_descriptor] = {
		.name  = "CUE_identifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},

	[component_name_descriptor] = {
		.name  = "component_name_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[logical_channel_number_descriptor] = {
		.name  = "logical_channel_number_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},

	[carousel_id_descriptor] = {
		.name  = "carousel_id_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[association_tag_descriptor] = {
		.name  = "association_tag_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[deferred_association_tags_descriptor] = {
		.name  = "deferred_association_tags_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},

	[hierarchical_transmission_descriptor] = {
		.name  = "hierarchical_transmission_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[digital_copy_control_descriptor] = {
		.name  = "digital_copy_control_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[network_identifier_descriptor] = {
		.name  = "network_identifier_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[partial_transport_stream_time_descriptor] = {
		.name  = "partial_transport_stream_time_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[audio_component_descriptor] = {
		.name  = "audio_component_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[hyperlink_descriptor] = {
		.name  = "hyperlink_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[target_area_descriptor] = {
		.name  = "target_area_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[data_contents_descriptor] = {
		.name  = "data_contents_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[video_decode_control_descriptor] = {
		.name  = "video_decode_control_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[download_content_descriptor] = {
		.name  = "download_content_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[CA_EMM_TS_descriptor] = {
		.name  = "CA_EMM_TS_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[CA_contract_information_descriptor] = {
		.name  = "CA_contract_information_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[CA_service_descriptor] = {
		.name  = "CA_service_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[TS_Information_descriptior] = {
		.name  = "TS_Information_descriptior",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[extended_broadcaster_descriptor] = {
		.name  = "extended_broadcaster_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[logo_transmission_descriptor] = {
		.name  = "logo_transmission_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[basic_local_event_descriptor] = {
		.name  = "basic_local_event_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[reference_descriptor] = {
		.name  = "reference_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[node_relation_descriptor] = {
		.name  = "node_relation_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[short_node_information_descriptor] = {
		.name  = "short_node_information_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[STC_reference_descriptor] = {
		.name  = "STC_reference_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[series_descriptor] = {
		.name  = "series_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[event_group_descriptor] = {
		.name  = "event_group_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[SI_parameter_descriptor] = {
		.name  = "SI_parameter_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[broadcaster_Name_Descriptor] = {
		.name  = "broadcaster_Name_Descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[component_group_descriptor] = {
		.name  = "component_group_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[SI_prime_TS_descriptor] = {
		.name  = "SI_prime_TS_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[board_information_descriptor] = {
		.name  = "board_information_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[LDT_linkage_descriptor] = {
		.name  = "LDT_linkage_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[connected_transmission_descriptor] = {
		.name  = "connected_transmission_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[content_availability_descriptor] = {
		.name  = "content_availability_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[service_group_descriptor] = {
		.name  = "service_group_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[carousel_compatible_composite_descriptor] = {
		.name  = "carousel_compatible_composite_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[conditional_playback_descriptor] = {
		.name  = "conditional_playback_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[ISDBT_delivery_system_descriptor] = {
		.name  = "ISDBT_delivery_system_descriptor",
		.init  = NULL,
		.print = NULL,
		.size  = 0,
		.free  = NULL,
	},
	[partial_reception_descriptor] = {
		.name  = "partial_reception_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[emergency_information_descriptor] = {
		.name  = "emergency_information_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[data_component_descriptor] = {
		.name  = "data_component_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[system_management_descriptor] = {
		.name  = "system_management_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_stuffing_descriptor] = {
		.name  = "atsc_stuffing_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_ac3_audio_descriptor] = {
		.name  = "atsc_ac3_audio_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_caption_service_descriptor] = {
		.name  = "atsc_caption_service_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_content_advisory_descriptor] = {
		.name  = "atsc_content_advisory_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_extended_channel_descriptor] = {
		.name  = "atsc_extended_channel_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_service_location_descriptor] = {
		.name  = "atsc_service_location_descriptor",
		.init  = atsc_desc_service_location_init,
		.print = atsc_desc_service_location_print,
		.free  = NULL,
		.size  = sizeof(struct atsc_desc_service_location),
	},
	[atsc_time_shifted_service_descriptor] = {
		.name  = "atsc_time_shifted_service_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_component_name_descriptor] = {
		.name  = "atsc_component_name_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_DCC_departing_request_descriptor] = {
		.name  = "atsc_DCC_departing_request_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_DCC_arriving_request_descriptor] = {
		.name  = "atsc_DCC_arriving_request_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_redistribution_control_descriptor] = {
		.name  = "atsc_redistribution_control_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_ATSC_private_information_descriptor] = {
		.name  = "atsc_ATSC_private_information_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
	[atsc_genre_descriptor] = {
		.name  = "atsc_genre_descriptor",
		.init  = NULL,
		.print = NULL,
		.free  = NULL,
		.size  = 0,
	},
};

uint32_t bcd(uint32_t bcd)
{
	uint32_t ret = 0, mult = 1;
	while (bcd) {
		ret += (bcd & 0x0f) * mult;
		bcd >>=4;
		mult *= 10;
	}
	return ret;
}

int bcd_to_int(const unsigned char *bcd, int bits)
{
	int nibble = 0;
	int ret = 0;

	while (bits) {
		ret *= 10;
		if (!nibble)
			ret += *bcd >> 4;
		else
			ret += *bcd & 0x0f;
		bits -= 4;
		nibble = !nibble;
		if (!nibble)
			bcd++;
	}
	return ret;
}

void hexdump(struct dvb_v5_fe_parms *parms, const char *prefix, const unsigned char *data, int length)
{
	if (!data)
		return;
	char ascii[17];
	char hex[50];
	int i, j = 0;
	hex[0] = '\0';
	for (i = 0; i < length; i++)
	{
		char t[4];
		snprintf (t, sizeof(t), "%02x ", (unsigned int) data[i]);
		strncat (hex, t, sizeof(hex));
		if (data[i] > 31 && data[i] < 128 )
			ascii[j] = data[i];
		else
			ascii[j] = '.';
		j++;
		if (j == 8)
			strncat(hex, " ", sizeof(hex));
		if (j == 16)
		{
			ascii[j] = '\0';
			dvb_log("%s%s  %s", prefix, hex, ascii);
			j = 0;
			hex[0] = '\0';
		}
	}
	if (j > 0 && j < 16)
	{
		char spaces[47];
		spaces[0] = '\0';
		for (i = strlen(hex); i < 49; i++)
			strncat(spaces, " ", sizeof(spaces));
		ascii[j] = '\0';
		dvb_log("%s%s %s %s", prefix, hex, spaces, ascii);
	}
}