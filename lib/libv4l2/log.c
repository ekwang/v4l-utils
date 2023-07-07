/*
#             (C) 2008 Elmar Kleijn <elmar_kleijn@hotmail.com>
#             (C) 2008 Sjoerd Piepenbrink <need4weed@gmail.com>
#             (C) 2008 Hans de Goede <hdegoede@redhat.com>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <execinfo.h>
#include "../libv4lconvert/libv4lsyscall-priv.h"
#if defined(__OpenBSD__)
#include <sys/videoio.h>
#else
#include <linux/videodev2.h>
#endif
#include "libv4l2.h"
#include "libv4l2-priv.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

FILE *v4l2_log_file = NULL;
int backtrace_depth = 0;

const char *v4l2_ioctls[] = {
	/* start v4l2 ioctls */
	[_IOC_NR(VIDIOC_QUERYCAP)]         = "VIDIOC_QUERYCAP",
	[_IOC_NR(VIDIOC_ENUM_FMT)]         = "VIDIOC_ENUM_FMT",
	[_IOC_NR(VIDIOC_G_FMT)]            = "VIDIOC_G_FMT",
	[_IOC_NR(VIDIOC_S_FMT)]            = "VIDIOC_S_FMT",
	[_IOC_NR(VIDIOC_REQBUFS)]          = "VIDIOC_REQBUFS",
	[_IOC_NR(VIDIOC_QUERYBUF)]         = "VIDIOC_QUERYBUF",
	[_IOC_NR(VIDIOC_G_FBUF)]           = "VIDIOC_G_FBUF",
	[_IOC_NR(VIDIOC_S_FBUF)]           = "VIDIOC_S_FBUF",
	[_IOC_NR(VIDIOC_OVERLAY)]          = "VIDIOC_OVERLAY",
	[_IOC_NR(VIDIOC_QBUF)]             = "VIDIOC_QBUF",
	[_IOC_NR(VIDIOC_EXPBUF)]           = "VIDIOC_EXPBUF",
	[_IOC_NR(VIDIOC_DQBUF)]            = "VIDIOC_DQBUF",
	[_IOC_NR(VIDIOC_STREAMON)]         = "VIDIOC_STREAMON",
	[_IOC_NR(VIDIOC_STREAMOFF)]        = "VIDIOC_STREAMOFF",
	[_IOC_NR(VIDIOC_G_PARM)]           = "VIDIOC_G_PARM",
	[_IOC_NR(VIDIOC_S_PARM)]           = "VIDIOC_S_PARM",
	[_IOC_NR(VIDIOC_G_STD)]            = "VIDIOC_G_STD",
	[_IOC_NR(VIDIOC_S_STD)]            = "VIDIOC_S_STD",
	[_IOC_NR(VIDIOC_ENUMSTD)]          = "VIDIOC_ENUMSTD",
	[_IOC_NR(VIDIOC_ENUMINPUT)]        = "VIDIOC_ENUMINPUT",
	[_IOC_NR(VIDIOC_G_CTRL)]           = "VIDIOC_G_CTRL",
	[_IOC_NR(VIDIOC_S_CTRL)]           = "VIDIOC_S_CTRL",
	[_IOC_NR(VIDIOC_G_TUNER)]          = "VIDIOC_G_TUNER",
	[_IOC_NR(VIDIOC_S_TUNER)]          = "VIDIOC_S_TUNER",
	[_IOC_NR(VIDIOC_G_AUDIO)]          = "VIDIOC_G_AUDIO",
	[_IOC_NR(VIDIOC_S_AUDIO)]          = "VIDIOC_S_AUDIO",
	[_IOC_NR(VIDIOC_QUERYCTRL)]        = "VIDIOC_QUERYCTRL",
	[_IOC_NR(VIDIOC_QUERYMENU)]        = "VIDIOC_QUERYMENU",
	[_IOC_NR(VIDIOC_G_INPUT)]          = "VIDIOC_G_INPUT",
	[_IOC_NR(VIDIOC_S_INPUT)]          = "VIDIOC_S_INPUT",
	[_IOC_NR(VIDIOC_G_EDID)]           = "VIDIOC_G_EDID",
	[_IOC_NR(VIDIOC_S_EDID)]           = "VIDIOC_S_EDID",
	[_IOC_NR(VIDIOC_G_OUTPUT)]         = "VIDIOC_G_OUTPUT",
	[_IOC_NR(VIDIOC_S_OUTPUT)]         = "VIDIOC_S_OUTPUT",
	[_IOC_NR(VIDIOC_ENUMOUTPUT)]       = "VIDIOC_ENUMOUTPUT",
	[_IOC_NR(VIDIOC_G_AUDOUT)]         = "VIDIOC_G_AUDOUT",
	[_IOC_NR(VIDIOC_S_AUDOUT)]         = "VIDIOC_S_AUDOUT",
	[_IOC_NR(VIDIOC_G_MODULATOR)]      = "VIDIOC_G_MODULATOR",
	[_IOC_NR(VIDIOC_S_MODULATOR)]      = "VIDIOC_S_MODULATOR",
	[_IOC_NR(VIDIOC_G_FREQUENCY)]      = "VIDIOC_G_FREQUENCY",
	[_IOC_NR(VIDIOC_S_FREQUENCY)]      = "VIDIOC_S_FREQUENCY",
	[_IOC_NR(VIDIOC_CROPCAP)]          = "VIDIOC_CROPCAP",
	[_IOC_NR(VIDIOC_G_CROP)]           = "VIDIOC_G_CROP",
	[_IOC_NR(VIDIOC_S_CROP)]           = "VIDIOC_S_CROP",
	[_IOC_NR(VIDIOC_G_JPEGCOMP)]       = "VIDIOC_G_JPEGCOMP",
	[_IOC_NR(VIDIOC_S_JPEGCOMP)]       = "VIDIOC_S_JPEGCOMP",
	[_IOC_NR(VIDIOC_QUERYSTD)]         = "VIDIOC_QUERYSTD",
	[_IOC_NR(VIDIOC_TRY_FMT)]          = "VIDIOC_TRY_FMT",
	[_IOC_NR(VIDIOC_ENUMAUDIO)]        = "VIDIOC_ENUMAUDIO",
	[_IOC_NR(VIDIOC_ENUMAUDOUT)]       = "VIDIOC_ENUMAUDOUT",
	[_IOC_NR(VIDIOC_G_PRIORITY)]       = "VIDIOC_G_PRIORITY",
	[_IOC_NR(VIDIOC_S_PRIORITY)]       = "VIDIOC_S_PRIORITY",
	[_IOC_NR(VIDIOC_G_SLICED_VBI_CAP)] = "VIDIOC_G_SLICED_VBI_CAP",
	[_IOC_NR(VIDIOC_LOG_STATUS)]       = "VIDIOC_LOG_STATUS",
	[_IOC_NR(VIDIOC_G_EXT_CTRLS)]      = "VIDIOC_G_EXT_CTRLS",
	[_IOC_NR(VIDIOC_S_EXT_CTRLS)]      = "VIDIOC_S_EXT_CTRLS",
	[_IOC_NR(VIDIOC_TRY_EXT_CTRLS)]    = "VIDIOC_TRY_EXT_CTRLS",
	[_IOC_NR(VIDIOC_ENUM_FRAMESIZES)]  = "VIDIOC_ENUM_FRAMESIZES",
	[_IOC_NR(VIDIOC_ENUM_FRAMEINTERVALS)] = "VIDIOC_ENUM_FRAMEINTERVALS",
	[_IOC_NR(VIDIOC_G_ENC_INDEX)]	   = "VIDIOC_G_ENC_INDEX",
	[_IOC_NR(VIDIOC_ENCODER_CMD)]	   = "VIDIOC_ENCODER_CMD",
	[_IOC_NR(VIDIOC_TRY_ENCODER_CMD)]  = "VIDIOC_TRY_ENCODER_CMD",
	[_IOC_NR(VIDIOC_DBG_S_REGISTER)]   = "VIDIOC_DBG_S_REGISTER",
	[_IOC_NR(VIDIOC_DBG_G_REGISTER)]   = "VIDIOC_DBG_G_REGISTER",
	[_IOC_NR(VIDIOC_S_HW_FREQ_SEEK)]   = "VIDIOC_S_HW_FREQ_SEEK",
	[_IOC_NR(VIDIOC_S_DV_TIMINGS)]	   = "VIDIOC_S_DV_TIMINGS",
	[_IOC_NR(VIDIOC_G_DV_TIMINGS)]	   = "VIDIOC_G_DV_TIMINGS",
	[_IOC_NR(VIDIOC_DQEVENT)]	   = "VIDIOC_DQEVENT",
	[_IOC_NR(VIDIOC_SUBSCRIBE_EVENT)]  = "VIDIOC_SUBSCRIBE_EVENT",
	[_IOC_NR(VIDIOC_UNSUBSCRIBE_EVENT)] = "VIDIOC_UNSUBSCRIBE_EVENT",
	[_IOC_NR(VIDIOC_CREATE_BUFS)]      = "VIDIOC_CREATE_BUFS",
	[_IOC_NR(VIDIOC_PREPARE_BUF)]      = "VIDIOC_PREPARE_BUF",
	[_IOC_NR(VIDIOC_G_SELECTION)]      = "VIDIOC_G_SELECTION",
	[_IOC_NR(VIDIOC_S_SELECTION)]      = "VIDIOC_S_SELECTION",
	[_IOC_NR(VIDIOC_DECODER_CMD)]      = "VIDIOC_DECODER_CMD",
	[_IOC_NR(VIDIOC_TRY_DECODER_CMD)]  = "VIDIOC_TRY_DECODER_CMD",
	[_IOC_NR(VIDIOC_ENUM_DV_TIMINGS)]  = "VIDIOC_ENUM_DV_TIMINGS",
	[_IOC_NR(VIDIOC_QUERY_DV_TIMINGS)] = "VIDIOC_QUERY_DV_TIMINGS",
	[_IOC_NR(VIDIOC_DV_TIMINGS_CAP)]   = "VIDIOC_DV_TIMINGS_CAP",
	[_IOC_NR(VIDIOC_ENUM_FREQ_BANDS)]  = "VIDIOC_ENUM_FREQ_BANDS",
	[_IOC_NR(VIDIOC_DBG_G_CHIP_INFO)]  = "VIDIOC_DBG_G_CHIP_INFO",
	[_IOC_NR(VIDIOC_QUERY_EXT_CTRL)]   = "VIDIOC_QUERY_EXT_CTRL",
};

static const char *v4l2_buf_type[] = {
	"",
	"V4L2_BUF_TYPE_VIDEO_CAPTURE",
	"V4L2_BUF_TYPE_VIDEO_OUTPUT",
	"V4L2_BUF_TYPE_VIDEO_OVERLAY",
	"V4L2_BUF_TYPE_VBI_CAPTURE",
	"V4L2_BUF_TYPE_VBI_OUTPUT",
	"V4L2_BUF_TYPE_SLICED_VBI_CAPTURE",
	"V4L2_BUF_TYPE_SLICED_VBI_OUTPUT",
	"V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY",
	"V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",
	"V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",
	"V4L2_BUF_TYPE_SDR_CAPTURE",
	"V4L2_BUF_TYPE_SDR_OUTPUT",
	"V4L2_BUF_TYPE_META_CAPTURE",
	"V4L2_BUF_TYPE_META_OUTPUT",
};

static const char *v4l2_memory[] = {
	"",
	"V4L2_MEMORY_MMAP",
	"V4L2_MEMORY_USERPTR",
	"V4L2_MEMORY_OVERLAY",
	"V4L2_MEMORY_DMABUF",
};

static const char *v4l2_decoder_cmd[] = {
	"V4L2_DEC_CMD_START",
	"V4L2_DEC_CMD_STOP",
	"V4L2_DEC_CMD_PAUSE",
	"V4L2_DEC_CMD_RESUME",
	"V4L2_DEC_CMD_FLUSH",
};

static const struct {
	unsigned int flag;
	const char *name;
} v4l2_capabilities[] = {
	{ V4L2_CAP_VIDEO_CAPTURE,        "VIDEO_CAPTURE" },
	{ V4L2_CAP_VIDEO_OUTPUT,         "VIDEO_OUTPUT" },
	{ V4L2_CAP_VIDEO_OVERLAY,        "VIDEO_OVERLAY" },
	{ V4L2_CAP_VBI_CAPTURE,          "VBI_CAPTURE" },
	{ V4L2_CAP_VBI_OUTPUT,           "VBI_OUTPUT" },
	{ V4L2_CAP_SLICED_VBI_CAPTURE,   "SLICED_VBI_CAPTURE" },
	{ V4L2_CAP_SLICED_VBI_OUTPUT,    "SLICED_VBI_OUTPUT" },
	{ V4L2_CAP_RDS_CAPTURE,          "RDS_CAPTURE" },
	{ V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "VIDEO_OUTPUT_OVERLAY" },
	{ V4L2_CAP_HW_FREQ_SEEK,         "HW_FREQ_SEEK" },
	{ V4L2_CAP_RDS_OUTPUT,           "RDS_OUTPUT" },
	{ V4L2_CAP_VIDEO_CAPTURE_MPLANE, "VIDEO_CAPTURE_MPLANE" },
	{ V4L2_CAP_VIDEO_OUTPUT_MPLANE,  "VIDEO_OUTPUT_MPLANE" },
	{ V4L2_CAP_VIDEO_M2M_MPLANE,     "VIDEO_M2M_MPLANE" },
	{ V4L2_CAP_VIDEO_M2M,            "VIDEO_M2M" },
	{ V4L2_CAP_TUNER,                "TUNER" },
	{ V4L2_CAP_AUDIO,                "AUDIO" },
	{ V4L2_CAP_RADIO,                "RADIO" },
	{ V4L2_CAP_MODULATOR,            "MODULATOR" },
	{ V4L2_CAP_SDR_CAPTURE,          "SDR_CAPTURE" },
	{ V4L2_CAP_EXT_PIX_FORMAT,       "EXT_PIX_FORMAT" },
	{ V4L2_CAP_SDR_OUTPUT,           "SDR_OUTPUT" },
	{ V4L2_CAP_META_CAPTURE,         "META_CAPTURE" },
	{ V4L2_CAP_READWRITE,            "READWRITE" },
	{ V4L2_CAP_ASYNCIO,              "ASYNCIO" },
	{ V4L2_CAP_STREAMING,            "STREAMING" },
	{ V4L2_CAP_META_OUTPUT,          "META_OUTPUT" },
	{ V4L2_CAP_TOUCH,                "TOUCH" },
	{ V4L2_CAP_IO_MC,                "IO_MC" },
	{ V4L2_CAP_DEVICE_CAPS,          "DEVICE_CAPS" },
};

#define SAFE_STR(arr, idx) \
	((unsigned)(idx) < ARRAY_SIZE(arr) && (arr)[idx] ? (arr)[idx] : "UNKNOWN")

static void log_v4l2_caps(const char *label, unsigned int caps)
{
	unsigned int known = 0;
	const char *sep = "";
	size_t i;

	fprintf(v4l2_log_file, "  %s: 0x%08x [", label, caps);
	for (i = 0; i < ARRAY_SIZE(v4l2_capabilities); i++) {
		if (caps & v4l2_capabilities[i].flag) {
			fprintf(v4l2_log_file, "%s%s", sep, v4l2_capabilities[i].name);
			sep = "|";
			known |= v4l2_capabilities[i].flag;
		}
	}
	if (caps & ~known)
		fprintf(v4l2_log_file, "%s0x%x", sep, caps & ~known);
	fprintf(v4l2_log_file, "]\n");
}

void callstack_dump(void)
{
	void *callstack[256];
	int i, nr_frames;
	char **strs;

	nr_frames = backtrace(callstack, ARRAY_SIZE(callstack));
	strs = backtrace_symbols(callstack, nr_frames);
	if (!strs)
		return;

	/* Skip this function and v4l2_log_ioctl. Print up to backtrace_depth
	   caller frames; report any frames truncated by the depth limit. */
	for (i = 2; i < nr_frames && (i - 2) < backtrace_depth; i++)
		fprintf(v4l2_log_file, "\t#%-3d%s\n", i, strs[i]);

	if (i < nr_frames)
		fprintf(v4l2_log_file, "\t...skip %d frames\n", nr_frames - i);

	free(strs);
}

void v4l2_log_ioctl(unsigned long int request, void *arg, int result)
{
	const char *ioctl_str;
	char buf[40];
	int saved_errno = errno;
	int buf_type = -1;

	if (!v4l2_log_file)
		return;

	if (_IOC_TYPE(request) == 'V' && _IOC_NR(request) < ARRAY_SIZE(v4l2_ioctls))
		ioctl_str = v4l2_ioctls[_IOC_NR(request)];
	else {
		snprintf(buf, sizeof(buf), "unknown request: %c %d",
				(int)_IOC_TYPE(request), (int)_IOC_NR(request));
		ioctl_str = buf;
	}

	fprintf(v4l2_log_file, "\n[ request == %s\n", ioctl_str);

	switch (request) {
	case VIDIOC_ENUM_FMT: {
		struct v4l2_fmtdesc *fmt = arg;
		int pixfmt = fmt->pixelformat;

		buf_type = fmt->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n", SAFE_STR(v4l2_buf_type, fmt->type), (int)fmt->type);
		fprintf(v4l2_log_file, "  index: %u, flag: 0x%X, description: %s\n",
				fmt->index, fmt->flags, (result < 0) ? "" : (const char *)fmt->description);
		fprintf(v4l2_log_file, "  pixelformat: %c%c%c%c\n",
				pixfmt & 0xff,
				(pixfmt >> 8) & 0xff,
				(pixfmt >> 16) & 0xff,
				pixfmt >> 24);
		break;
	}
	case VIDIOC_G_FMT:
	case VIDIOC_S_FMT:
	case VIDIOC_TRY_FMT: {
		struct v4l2_format *fmt = arg;
		int pixfmt = fmt->fmt.pix.pixelformat;
		int i = 0;

		buf_type = fmt->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n", SAFE_STR(v4l2_buf_type, fmt->type), (int)fmt->type);
		if (fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			fprintf(v4l2_log_file, "  pixelformat: %c%c%c%c %ux%u\n",
					pixfmt & 0xff,
					(pixfmt >> 8) & 0xff,
					(pixfmt >> 16) & 0xff,
					pixfmt >> 24,
					fmt->fmt.pix.width,
					fmt->fmt.pix.height);
			fprintf(v4l2_log_file, "  field: %d, bytesperline: %d, sizeimage: %d\n",
					(int)fmt->fmt.pix.field, (int)fmt->fmt.pix.bytesperline,
					(int)fmt->fmt.pix.sizeimage);
			fprintf(v4l2_log_file, "  colorspace: %d, priv: %x\n",
					(int)fmt->fmt.pix.colorspace, (int)fmt->fmt.pix.priv);
		}
		else if (fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
				|| fmt->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
			struct v4l2_pix_format_mplane *pix_mp = &fmt->fmt.pix_mp;

			pixfmt = pix_mp->pixelformat;
			fprintf(v4l2_log_file, "  pixelformat: %c%c%c%c %ux%u\n",
					pixfmt & 0xff,
					(pixfmt >> 8) & 0xff,
					(pixfmt >> 16) & 0xff,
					pixfmt >> 24,
					pix_mp->width,
					pix_mp->height);
			fprintf(v4l2_log_file, "  num_planes: %d, flags: 0x%X, field: %d, colorspace: %d\n",
					(int)pix_mp->num_planes, pix_mp->flags,
					(int)pix_mp->field, (int)pix_mp->colorspace);

			for (i = 0; i < pix_mp->num_planes; i++)
			{
				fprintf(v4l2_log_file, "  [%d] bytesperline: %d, sizeimage: %d\n",
						i,
						(int)pix_mp->plane_fmt[i].bytesperline,
						(int)pix_mp->plane_fmt[i].sizeimage);
			}
		}
		break;
	}
	case VIDIOC_REQBUFS: {
		struct v4l2_requestbuffers *req = arg;

		buf_type = req->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n", SAFE_STR(v4l2_buf_type, req->type), (int)req->type);
		fprintf(v4l2_log_file, "  count: %u, memory: %s(%d)\n",
				req->count, SAFE_STR(v4l2_memory, req->memory), (int)req->memory);
		break;
	}
	case VIDIOC_QUERYBUF:
	case VIDIOC_QBUF:
	case VIDIOC_DQBUF: {
		int i = 0;
		struct v4l2_buffer *buf = arg;

		buf_type = buf->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n",
				SAFE_STR(v4l2_buf_type, buf->type), buf->type);
		fprintf(v4l2_log_file, "  index: %u, bytesused: %u, flags: 0x%X\n",
				buf->index, buf->bytesused, buf->flags);
		fprintf(v4l2_log_file, "  timestamp %ld.%06ld\n",
				(long)buf->timestamp.tv_sec,
				(long)buf->timestamp.tv_usec);
		if (buf->flags & V4L2_BUF_FLAG_TIMECODE) {
			struct v4l2_timecode *tc = &buf->timecode;
			fprintf(v4l2_log_file, "  timecode type: %u, flags: 0x%x, %02u:%02u:%02u.%02u\n",
					tc->type, tc->flags,
					tc->hours, tc->minutes, tc->seconds, tc->frames);
		}
		fprintf(v4l2_log_file, "  sequence: %u, length: %u\n",
				buf->sequence, buf->length);
		fprintf(v4l2_log_file, "  memory: %s(%u)\n",
				SAFE_STR(v4l2_memory, buf->memory), buf->memory);
		if (V4L2_TYPE_IS_MULTIPLANAR(buf->type)) {
			for (i = 0; i < buf->length; i++) {
				fprintf(v4l2_log_file, "  planes[%d] bytesused: %u, length: %u, data_offset: %u",
						i,
						buf->m.planes[i].bytesused,
						buf->m.planes[i].length,
						buf->m.planes[i].data_offset);
				if (buf->memory == V4L2_MEMORY_MMAP)
					fprintf(v4l2_log_file, " mem_offset: %u\n", buf->m.planes[i].m.mem_offset);
				else if (buf->memory == V4L2_MEMORY_USERPTR)
					fprintf(v4l2_log_file, " userptr: 0x%lx\n", buf->m.planes[i].m.userptr);
				else if (buf->memory == V4L2_MEMORY_DMABUF)
					fprintf(v4l2_log_file, " fd: %d\n", buf->m.planes[i].m.fd);
				else
					fprintf(v4l2_log_file, "\n");
			}
		} else {
			if (buf->memory == V4L2_MEMORY_MMAP)
				fprintf(v4l2_log_file, "  offset: %u\n", buf->m.offset);
			else if (buf->memory == V4L2_MEMORY_USERPTR)
				fprintf(v4l2_log_file, "  userptr: 0x%lx\n", buf->m.userptr);
			else if (buf->memory == V4L2_MEMORY_DMABUF)
				fprintf(v4l2_log_file, "  fd: %d\n", buf->m.fd);
		}
		break;
	}
	case VIDIOC_ENUM_FRAMESIZES: {
		struct v4l2_frmsizeenum *frmsize = arg;
		int pixfmt = frmsize->pixel_format;

		fprintf(v4l2_log_file, "  frm_type: %d\n", (int)frmsize->type);
		fprintf(v4l2_log_file, "  index: %u, pixelformat: %c%c%c%c\n",
				frmsize->index,
				pixfmt & 0xff,
				(pixfmt >> 8) & 0xff,
				(pixfmt >> 16) & 0xff,
				pixfmt >> 24);
		switch (frmsize->type) {
		case V4L2_FRMSIZE_TYPE_DISCRETE:
			fprintf(v4l2_log_file, "  %ux%u\n", frmsize->discrete.width,
					frmsize->discrete.height);
			break;
		case V4L2_FRMSIZE_TYPE_CONTINUOUS:
		case V4L2_FRMSIZE_TYPE_STEPWISE:
			fprintf(v4l2_log_file, "  %ux%u -> %ux%u, step:(%ux%u)\n",
					frmsize->stepwise.min_width, frmsize->stepwise.min_height,
					frmsize->stepwise.max_width, frmsize->stepwise.max_height,
					frmsize->stepwise.step_width, frmsize->stepwise.step_height);
			break;
		}
		break;
	}
	case VIDIOC_ENUM_FRAMEINTERVALS: {
		struct v4l2_frmivalenum *frmival = arg;
		int pixfmt = frmival->pixel_format;

		fprintf(v4l2_log_file, "  frm_type: %d\n", (int)frmival->type);
		fprintf(v4l2_log_file, "  index: %u, pixelformat: %c%c%c%c %ux%u:\n",
				frmival->index,
				pixfmt & 0xff,
				(pixfmt >> 8) & 0xff,
				(pixfmt >> 16) & 0xff,
				pixfmt >> 24,
				frmival->width,
				frmival->height);
		switch (frmival->type) {
		case V4L2_FRMIVAL_TYPE_DISCRETE:
			fprintf(v4l2_log_file, "  %u/%u\n", frmival->discrete.numerator,
					frmival->discrete.denominator);
			break;
		case V4L2_FRMIVAL_TYPE_CONTINUOUS:
		case V4L2_FRMIVAL_TYPE_STEPWISE:
			fprintf(v4l2_log_file, "  %u/%u -> %u/%u\n",
					frmival->stepwise.min.numerator,
					frmival->stepwise.min.denominator,
					frmival->stepwise.max.numerator,
					frmival->stepwise.max.denominator);
			break;
		}
		break;
	}
	case VIDIOC_G_PARM:
	case VIDIOC_S_PARM: {
		struct v4l2_streamparm *parm = arg;

		buf_type = parm->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n", SAFE_STR(v4l2_buf_type, parm->type), (int)parm->type);

		if (parm->type == V4L2_BUF_TYPE_VIDEO_CAPTURE
				|| parm->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
			fprintf(v4l2_log_file, "  capability: 0x%x, capturemode: %u, \
          extendedmode: %u, readbuffers: %u\n",
					parm->parm.capture.capability, parm->parm.capture.capturemode,
					parm->parm.capture.extendedmode, parm->parm.capture.readbuffers);
			if (parm->parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
				fprintf(v4l2_log_file, "timeperframe: %u/%u\n",
						parm->parm.capture.timeperframe.numerator,
						parm->parm.capture.timeperframe.denominator);
			}
		}
		else if(parm->type == V4L2_BUF_TYPE_VIDEO_OUTPUT
				|| parm->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
			fprintf(v4l2_log_file, "  capability: 0x%x, outputmode: %u, \
          extendedmode: %u, writebuffers: %u\n",
					parm->parm.output.capability, parm->parm.output.outputmode,
					parm->parm.output.extendedmode, parm->parm.output.writebuffers);
			fprintf(v4l2_log_file, "timeperframe: %u/%u\n",
				parm->parm.output.timeperframe.numerator,
				parm->parm.output.timeperframe.denominator);
		}
		break;
	}
	case VIDIOC_QUERYCAP: {
		struct v4l2_capability *cap = arg;

		fprintf(v4l2_log_file, "  driver: %s, card: %s, bus_info: %s, version: %u.%u.%u\n",
				cap->driver, cap->card, cap->bus_info,
				(cap->version >> 16) & 0xff,
				(cap->version >> 8) & 0xff,
				cap->version & 0xff);
		log_v4l2_caps("capabilities", cap->capabilities);
		log_v4l2_caps("device_caps", cap->device_caps);
		break;
	}
	case VIDIOC_CROPCAP: {
		struct v4l2_cropcap *crop = arg;

		buf_type = crop->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n", SAFE_STR(v4l2_buf_type, crop->type), (int)crop->type);
		fprintf(v4l2_log_file, "  bounds left: %d, top: %d, width: %d, height: %d\n",
				crop->bounds.left, crop->bounds.top, crop->bounds.width, crop->bounds.height);
		fprintf(v4l2_log_file, "  defrect left: %d, top: %d, width: %d, height: %d\n",
				crop->defrect.left, crop->defrect.top, crop->defrect.width, crop->defrect.height);
		fprintf(v4l2_log_file, "  pixelaspect numerator: %d, denominator: %d\n",
				crop->pixelaspect.numerator, crop->pixelaspect.denominator);
		break;
	}
	case VIDIOC_G_SELECTION: {
		struct v4l2_selection *sel = arg;

		buf_type = sel->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n", SAFE_STR(v4l2_buf_type, sel->type), (int)sel->type);
		fprintf(v4l2_log_file, "  target: %d, flags:0x%x\n", (int)sel->target, sel->flags);
		fprintf(v4l2_log_file, "  rect left: %d, top: %d, width: %d, height: %d\n",
				sel->r.left, sel->r.top, sel->r.width, sel->r.height);
		break;
	}
	case VIDIOC_EXPBUF: {
		struct v4l2_exportbuffer *exp = arg;

		buf_type = exp->type;
		fprintf(v4l2_log_file, "  buf_type: %s(%u)\n", SAFE_STR(v4l2_buf_type, exp->type), (int)exp->type);
		fprintf(v4l2_log_file, "  index: %u, plane: %u, flags: 0x%X, fd: %d\n", (int)exp->index, exp->plane, exp->flags, exp->fd);
		break;
	}
	case VIDIOC_DECODER_CMD: {
		struct v4l2_decoder_cmd *dcmd = arg;

		fprintf(v4l2_log_file, "  cmd: %s(%u), flags: 0x%x\n",
				SAFE_STR(v4l2_decoder_cmd, dcmd->cmd), dcmd->cmd, dcmd->flags);
		break;
	}
	}

	if (result < 0)
		fprintf(v4l2_log_file, "] %s (buf_type:%d) result == %d (errno:%d, %s)\n", ioctl_str, buf_type, result, saved_errno, strerror(saved_errno));
	else
		fprintf(v4l2_log_file, "] %s (buf_type:%d) result == %d\n", ioctl_str, buf_type, result);

	if (backtrace_depth)
		callstack_dump();

	fflush(v4l2_log_file);
}

