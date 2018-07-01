#include <cstdio>
#include <algorithm>
#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/file.h>
}
/*
	 1. ä»Žå†…å­˜è¯»å–MP3/WAV
	 2. ä»Žå†…å­˜è¯»å–veidoã€‚ï¼ˆè¿™é‡Œç”¨éšæœºç”Ÿæˆçš„yuvæ•°æ®ä»£æ›¿ï¼‰
 */

struct OutputStream {
	AVStream *st;
	AVCodecContext *enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
};

const char * mp3 = "1017.mp3";

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

void runDemo();
void genVideo();
void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
void readAudio();
void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id);
int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt);
AVFrame *get_audio_frame(OutputStream *ost);
int write_audio_frame(AVFormatContext *oc, OutputStream *ost);
int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);

int main() {
	runDemo();
	printf("muxing demo\n");
	return 0;
}

struct buffer_data {
	uint8_t *ptr;
	size_t len;
};


void runDemo() {
	readAudio();
	genVideo();
}

void genVideo() {
	OutputStream video_st, audio_st;
	const char *filename = "gen.mp4";
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVCodec *audio_codec, *video_codec;
	int ret;
	AVDictionary *opt = NULL;
	avformat_alloc_output_context2(&oc, NULL, NULL, filename);
	if (!oc) {
		printf("failed to open context2\n");
		return;
	}
	fmt = oc->oformat;
	//printf("video codec: %d\n", fmt->video_codec);
	//printf("audio codec: %d\n", fmt->audio_codec);
	if (fmt->video_codec != AV_CODEC_ID_NONE) {
		add_stream(&video_st, oc, &video_codec, fmt->video_codec);
		open_video(oc, video_codec, &video_st, opt);
	}

	if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
		open_audio(oc, audio_codec, &audio_st, opt);
	}

	ret = avformat_write_header(oc, &opt);
	if (ret < 0) {
		printf("write header failed.");
		return;
	}

	int encode_video = 1, encode_audio = 1;
	while (encode_video || encode_audio) {
		encode_audio = 1;

	}
	av_write_trailer(oc);

	return;
}

int read_packet(void *opaque, uint8_t *buf, int buf_size) {
	buffer_data *bd = (buffer_data *) opaque;
	//printf("buf_size = %d len=%d\n", buf_size, bd->len);
	buf_size = FFMIN(buf_size, bd->len);
	if (!buf_size) {
		printf("%d end\n", buf_size);
		return AVERROR_EOF;
	}
	printf("ptr size: %d\n", bd->len);
	memcpy(buf, bd->ptr, buf_size);
	bd->ptr += buf_size;
	bd->len -= buf_size;
	return buf_size;
}

void readAudio() {
	FILE *fp;
	fp = fopen(mp3, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Failed to read mp3 file: %s\n", mp3);
		return;
	}
	fseek(fp, 0L, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	buffer_data bd;
	bd.ptr = new uint8_t[size];
	bd.len = size;
	uint8_t *buf = bd.ptr;
	int ret = 0, once = 4096;
	ret = fread(bd.ptr, 1, size, fp);
	printf("read ret: %d\n", ret);
	/*
		 while (!feof(fp)) {
		 ret = fread(bd.ptr, 1, once, fp);
		 size -= ret;
		 buf += ret;
		 printf("read size: %d %d %d\n",ret, size, buf - bd.ptr);
		 }
	 */
	/*
		 FILE *wp = fopen("gen.mp3", "wb");
		 ret = fwrite(bd.ptr, 1, bd.len, wp);
		 fclose(wp);
		 printf("write ret: %d\n", ret);
	 */
	/*
		 ret = fread(bd.ptr, 1, once, fp);
		 size -= ret;
		 buf += ret;
		 printf("read size: %d %d %d\n",ret, size, buf - bd.ptr);
		 }
	 */
	/*
		 FILE *wp = fopen("gen.mp3", "wb");
		 ret = fwrite(bd.ptr, 1, bd.len, wp);
		 fclose(wp);
		 printf("write ret: %d\n", ret);
	 */
	/*
		 ret = fread(bd.ptr, 1, once, fp);
		 size -= ret;
		 buf += ret;
		 printf("read size: %d %d %d\n",ret, size, buf - bd.ptr);
		 }
	 */
	/*
		 FILE *wp = fopen("gen.mp3", "wb");
		 ret = fwrite(bd.ptr, 1, bd.len, wp);
		 fclose(wp);
		 printf("write ret: %d\n", ret);
	 */
	/*
		 ret = fread(bd.ptr, 1, once, fp);
		 size -= ret;
		 buf += ret;
		 printf("read size: %d %d %d\n",ret, size, buf - bd.ptr);
		 }
	 */
	/*
		 FILE *wp = fopen("gen.mp3", "wb");
		 ret = fwrite(bd.ptr, 1, bd.len, wp);
		 fclose(wp);
		 printf("write ret: %d\n", ret);
	 */
	/*
		 printf("read ret: %d\n", ret);
	/*
	while (!feof(fp)) {
	ret = fread(bd.ptr, 1, once, fp);
	size -= ret;
	buf += ret;
	printf("read size: %d %d %d\n",ret, size, buf - bd.ptr);
	}
	 */
	/*
		 FILE *wp = fopen("gen.mp3", "wb");
		 ret = fwrite(bd.ptr, 1, bd.len, wp);
		 fclose(wp);
		 printf("write ret: %d\n", ret);
	 */
	/*
		 uint8_t *buffer = NULL;
		 size_t buffer_size;
		 ret = av_file_map(mp3, &buffer, &buffer_size, 0, NULL);
		 if (ret < 0) {
		 printf("file to map file");
		 return;
		 }
		 bd.ptr = buffer;
		 bd.len = buffer_size;
	 */
	if (size == 0) {
		printf("read all file %d len=%d\n", ret, bd.len);
	} else {
		//printf("failed to read file %d\n", size);
		//return;
	}
// åˆ†é…ä¸´æ—¶å†…å­˜
uint8_t *av_ctx_buffer;
size_t av_buffer_size = 4096;
av_ctx_buffer = (uint8_t*) av_malloc(av_buffer_size);
AVIOContext *avio = avio_alloc_context(av_ctx_buffer,av_buffer_size, 0, &bd, &read_packet, NULL, NULL);
if (!avio) {
	printf("open avio failed\n");
	return;
}
AVFormatContext *fmt_ctx = avformat_alloc_context();
fmt_ctx->pb = avio;
ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
if (ret < 0) {
	fprintf(stderr,"failed to open input %d\n", ret);
	//return;
}
ret = avformat_find_stream_info(fmt_ctx, NULL);
if (ret < 0) {
	fprintf(stderr,"failed to find stream %d\n", ret);
}
av_dump_format(fmt_ctx, 0, mp3, 0);
}

int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
	int ret;

	*got_frame = 0;

	if (pkt) {
		ret = avcodec_send_packet(avctx, pkt);
		// In particular, we don't expect AVERROR(EAGAIN), because we read all
		// decoded frames with avcodec_receive_frame() until done.
		if (ret < 0 && ret != AVERROR_EOF)
			return ret;
	}

	ret = avcodec_receive_frame(avctx, frame);
	if (ret < 0 && ret != AVERROR(EAGAIN))
		return ret;
	if (ret >= 0)
		*got_frame = 1;

	return 0;
}	

void add_stream(OutputStream *ost, AVFormatContext *oc,
		AVCodec **codec,
		enum AVCodecID codec_id)
{
	AVCodecContext *c;
	int i;

	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		fprintf(stderr, "Could not find encoder for '%s'\n",
				avcodec_get_name(codec_id));
		exit(1);
	}

	ost->st = avformat_new_stream(oc, NULL);
	if (!ost->st) {
		fprintf(stderr, "Could not allocate stream\n");
		exit(1);
	}
	ost->st->id = oc->nb_streams-1;
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		fprintf(stderr, "Could not alloc an encoding context\n");
		exit(1);
	}
	ost->enc = c;

	switch ((*codec)->type) {
		case AVMEDIA_TYPE_AUDIO:
			c->sample_fmt  = (*codec)->sample_fmts ?
				(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
			c->bit_rate    = 64000;
			c->sample_rate = 44100;
			if ((*codec)->supported_samplerates) {
				c->sample_rate = (*codec)->supported_samplerates[0];
				for (i = 0; (*codec)->supported_samplerates[i]; i++) {
					if ((*codec)->supported_samplerates[i] == 44100)
						c->sample_rate = 44100;
				}
			}
			c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
			c->channel_layout = AV_CH_LAYOUT_STEREO;
			if ((*codec)->channel_layouts) {
				c->channel_layout = (*codec)->channel_layouts[0];
				for (i = 0; (*codec)->channel_layouts[i]; i++) {
					if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
						c->channel_layout = AV_CH_LAYOUT_STEREO;
				}
			}
			c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
			ost->st->time_base = (AVRational){ 1, c->sample_rate };
			break;

		case AVMEDIA_TYPE_VIDEO:
			c->codec_id = codec_id;

			c->bit_rate = 400000;
			/* Resolution must be a multiple of two. */
			c->width    = 352;
			c->height   = 288;
			/* timebase: This is the fundamental unit of time (in seconds) in terms
			 * of which frame timestamps are represented. For fixed-fps content,
			 * timebase should be 1/framerate and timestamp increments should be
			 * identical to 1. */
			ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
			c->time_base       = ost->st->time_base;

			c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
			c->pix_fmt       = STREAM_PIX_FMT;
			if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
				/* just for testing, we also add B-frames */
				c->max_b_frames = 2;
			}
			if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
				/* Needed to avoid using macroblocks in which some coeffs overflow.
				 * This does not happen with normal video, it just happens here as
				 * the motion of the chroma plane does not match the luma plane. */
				c->mb_decision = 2;
			}
			break;

		default:
			break;
	}

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
		uint64_t channel_layout,
		int sample_rate, int nb_samples)
{
	AVFrame *frame = av_frame_alloc();
	int ret;

	if (!frame) {
		fprintf(stderr, "Error allocating an audio frame\n");
		exit(1);
	}

	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;

	if (nb_samples) {
		ret = av_frame_get_buffer(frame, 0);
		if (ret < 0) {
			fprintf(stderr, "Error allocating an audio buffer\n");
			exit(1);
		}
	}

	return frame;
}

void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
	AVCodecContext *c;
	int nb_samples;
	int ret;
	AVDictionary *opt = NULL;

	c = ost->enc;

	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		printf("could not open audio codec: %d\n", ret);
		return;
	}

	ost->t     = 0;
	ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;

	ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
			c->sample_rate, nb_samples);
	ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
			c->sample_rate, nb_samples);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}

	/* create resampler context */
	ost->swr_ctx = swr_alloc();
	if (!ost->swr_ctx) {
		fprintf(stderr, "Could not allocate resampler context\n");
		exit(1);
	}

	/* set options */
	av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
	av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
	av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

	/* initialize the resampling context */
	if ((ret = swr_init(ost->swr_ctx)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		exit(1);
	}
}

AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture)
		return NULL;

	picture->format = pix_fmt;
	picture->width  = width;
	picture->height = height;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate frame data.\n");
		exit(1);
	}

	return picture;
}

void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	int ret;
	AVCodecContext *c = ost->enc;
	AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	/* open the codec */
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		fprintf(stderr, "Could not open video codec: %d\n", ret);
		exit(1);
	}

	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!ost->frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	 * picture is needed too. It is then converted to the required
	 * output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!ost->tmp_frame) {
			fprintf(stderr, "Could not allocate temporary picture\n");
			exit(1);
		}
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}
}

AVFrame *get_audio_frame(OutputStream *ost)
{
	AVFrame *frame = ost->tmp_frame;
	int j, i, v;
	int16_t *q = (int16_t*)frame->data[0];

	/* check if we want to generate more frames */
	if (av_compare_ts(ost->next_pts, ost->enc->time_base,
				STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
		return NULL;

	for (j = 0; j <frame->nb_samples; j++) {
		v = (int)(sin(ost->t) * 10000);
		for (i = 0; i < ost->enc->channels; i++)
			*q++ = v;
		ost->t     += ost->tincr;
		ost->tincr += ost->tincr2;
	}

	frame->pts = ost->next_pts;
	ost->next_pts  += frame->nb_samples;

	return frame;
}

int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
	AVCodecContext *c;
	AVPacket pkt = { 0 }; // data and size must be 0;
	AVFrame *frame;
	int ret;
	int got_packet;
	int dst_nb_samples;

	av_init_packet(&pkt);
	c = ost->enc;

	frame = get_audio_frame(ost);

	if (frame) {
		/* convert samples from native format to destination codec format, using the resampler */
		/* compute destination number of samples */
		dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
				c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);

		/* when we pass a frame to the encoder, it may keep a reference to it
		 * internally;
		 * make sure we do not overwrite it here
		 */
		ret = av_frame_make_writable(ost->frame);
		if (ret < 0)
			exit(1);

		/* convert to destination format */
		ret = swr_convert(ost->swr_ctx,
				ost->frame->data, dst_nb_samples,
				(const uint8_t **)frame->data, frame->nb_samples);
		if (ret < 0) {
			fprintf(stderr, "Error while converting\n");
			exit(1);
		}
		frame = ost->frame;

		frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
		ost->samples_count += dst_nb_samples;
	}

	//ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
	ret = decode(c, frame, &got_packet, &pkt);
	if (ret < 0) {
		fprintf(stderr, "Error encoding audio frame: %d\n", ret);
		exit(1);
	}

	if (got_packet) {
		ret = write_frame(oc, &c->time_base, ost->st, &pkt);
		if (ret < 0) {
			fprintf(stderr, "Error while writing audio frame: %d\n",ret);
			exit(1);
		}
	}

	return (frame || got_packet) ? 0 : 1;
}

int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(fmt_ctx, pkt);
}
