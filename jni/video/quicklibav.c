//
//  quicklibav.c
//  quick functions for libav
//
//  Created by star king on 12-5-15.
//  Copyright (c) 2012年 elegant cloud. All rights reserved.
//
#include "../common.h"
#include "quicklibav.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"

static unsigned char *video_outbuf = NULL;
static int video_outbuf_size = 20000;


AVFrame * alloc_picture(enum PixelFormat pix_fmt, int width, int height) {
    AVFrame *picture = avcodec_alloc_frame();
    if (!picture) {
        return NULL;
    }
    
    av_image_alloc(picture->data, picture->linesize, width, height, pix_fmt, 1);
    return picture;
}


/**
 * find specified encoder and create new stream
 * And open the codec
 */
static AVStream * create_video_stream(AVFormatContext *oc, enum CodecID codec_id, int width, int height) {
    AVCodecContext *c;
    AVStream *st;
    AVCodec *codec;
    
    /* find the video encoder */
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        D("codec not found\n");
        return NULL;
    }
    
    st = avformat_new_stream(oc, codec);
    if (!st) {
        D("Could not alloc stream\n");
        return NULL;
    }
    
    D("new video stream time base: %d / %d\n", st->time_base.num, st->time_base.den);
    
    c = st->codec;
    
    /**** set codec parameters ****/
    /* resolution must be a multiple of two */
    c->width = width;
    c->height = height;
    /* frames per second */
    c->time_base = (AVRational){1, STREAM_FRAME_RATE};
    c->gop_size = 12; /* emit one intra frame every 12 frames */

    c->pix_fmt = STREAM_PIX_FMT;
    c->level = 10; //Level 
    
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    
    AVDictionary *dict = 0;
    av_dict_set(&dict, "profile", "baseline", 0);

    /* open video codec */
    if (avcodec_open2(c, codec, &dict) < 0) {
        D("Could not open codec");
        av_dict_free(&dict);
        return NULL;
    }
    
    av_dict_free(&dict);
    
    return st;
}

int init_quick_video_output(QuickVideoOutput *qvo, const char *filename, const char *type) {
    int ret = 0;
    if (qvo == NULL) {
        return -1;
    }
    
    av_register_all();
   
    avformat_network_init();
    
    AVOutputFormat *fmt = av_guess_format(type, filename, NULL);
    if (!fmt) {
        D("Could not find suitable output format!\n");
        qvo->initSuccessFlag = 0;
        return -1;
    }
    
    /* allocate the output media context */
    AVFormatContext *oc = avformat_alloc_context();
    if (!oc) {
        D("Memory error when allocating AVFormatContext!\n");
        qvo->initSuccessFlag = 0;
        return -1;
    }
    
    fmt->video_codec = CODEC_ID_H264;
    oc->oformat = fmt;
    snprintf(oc->filename, sizeof(oc->filename), "%s", filename);
    qvo->video_output_context = oc;
    
    AVStream *video_st = create_video_stream(oc, fmt->video_codec, qvo->width, qvo->height);

    if (!video_st) {
        D("Could not add video stream\n");
        qvo->initSuccessFlag = 0;
        return -1;
    }
    qvo->video_stream = video_st;
    
    /* init out buffer */
    video_outbuf_size = 100000 + 12 * video_st->codec->width * video_st->codec->height;
    video_outbuf = av_malloc(video_outbuf_size);
    
    av_dump_format(oc, 0, filename, 1);
    
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        D("try to open file: %s\n", filename);
        if (avio_open(&oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
            D("Could not open '%s'\n", filename);
            qvo->initSuccessFlag = 0;
            return -1;
        } else {
            D("file open ok\n");
        }
    }
    /* write the stream header, if any */
    avformat_write_header(oc, NULL);
    
    qvo->initSuccessFlag = 1;
    
    return ret;
}


void close_quick_video_ouput(QuickVideoOutput *qvo) {
    if (!qvo) {
        return;
    }
    
    AVFormatContext *oc = qvo->video_output_context;
    AVStream *st = qvo->video_stream;
    
    if (qvo->initSuccessFlag && oc) {
        av_write_trailer(oc);
    }
     
    avformat_network_deinit();
    
    if (st && st->codec) {
        avcodec_close(st->codec);
    }
    
    if (video_outbuf) {
        av_free(video_outbuf);
    }
    
    if (oc) {
        /* free the streams */
    	int i = 0;
        for (i = 0; i < oc->nb_streams; i++) {
            av_freep(&oc->streams[i]->codec);
            av_freep(&oc->streams[i]);
        }
        if (!(oc->oformat->flags & AVFMT_NOFILE)) {
            /* close the output file */
            D("close output file %s\n", oc->filename);
            avio_close(oc->pb);
        }
        
        /* free the stream */
        av_free(oc);
    }
    
    
}


int write_video_frame(QuickVideoOutput *qvo, AVFrame *raw_picture) {
    if (!qvo || !qvo->video_stream || !qvo->video_output_context) {
        return -1;
    }
    
    AVCodecContext *c = qvo->video_stream->codec;
    if (!c) {
        return -1;
    }
    
    AVFormatContext *oc = qvo->video_output_context;
    
    int ret = 0;
    
    /* encode the image */
    int out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, raw_picture);
    /* if size is zero, it means the image was buffered */
    if (out_size > 0) {
        AVPacket pkt;
        av_init_packet(&pkt);
        
        if (c->coded_frame->pts != AV_NOPTS_VALUE) {
            pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, qvo->video_stream->time_base);
        }
        
        if (c->coded_frame->key_frame) {
            pkt.flags |= AV_PKT_FLAG_KEY;
        }
        
        pkt.stream_index = qvo->video_stream->index;
        pkt.data = video_outbuf;
        pkt.size = out_size;
    
        /*
        printf("pkt pts: %lld c->time_base: %d / %d vt->time_base: %d / %d\n", pkt.pts, c->time_base.num, c->time_base.den, qvo->video_stream->time_base.num, qvo->video_stream->time_base.den);
        */
        
        /* wite the compressed frame to the media file */
        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        ret = 0;
    }
    
    if (ret != 0) {
        D("error while writing video frame\n");
        return -2;
    }
    
    return out_size;
    
}


