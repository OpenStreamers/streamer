

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "xchannel.h"



static int xchannel_debug = 3;
#ifdef MODULE_NAME
#undef MODULE_NAME
#endif

#define MODULE_NAME     "xchannel"

#define DEB1(format, arg...)    \
    if (xchannel_debug >=1) {    \
        printf(MODULE_NAME "\t F: %s"" L: %u " format, __FUNCTION__ ,__LINE__,  ## arg );\
    }
#define DEB2(format, arg...)    \
    if (xchannel_debug >=2) {    \
        printf(MODULE_NAME "\t F: %s"" L: %u " format, __FUNCTION__ ,__LINE__,  ## arg );\
    }
#define DEB3(format, arg...)    \
    if (xchannel_debug >=3) {    \
        printf(MODULE_NAME "\t F: %s"" L: %u " format, __FUNCTION__ ,__LINE__,  ## arg );\
    }


#define ERR(format, arg...)    printf("ERROR: "MODULE_NAME " F: %s"" L: %u " format, __FUNCTION__ ,__LINE__,  ## arg )

#define LOG(format, arg...)    printf("LOG: "MODULE_NAME ": " format, ## arg )


#define VAL_IN_RANGE(val, min, max) ((val)<(min) ? (min):((val)>(max) ? (max):(val))) 
// if  in range val is true
#define VAL_IF_IN_RANGE(val, min, max) ((val)<(min) ? 0:((val)>(max) ? 0:1)) 

Fifo_Attrs Fifo_Attrs_DEFAULT = {
    0
};



/******************************************************************************
 * Fifo_delete
 ******************************************************************************/
int Fifo_delete(Fifo_Handle hFifo)
{
    int ret = Mod_EOK;

    if (hFifo) {
        if (close(hFifo->pipes[0])) {
            ret = Mod_EIO;
        }

        if (close(hFifo->pipes[1])) {
            ret = Mod_EIO;
        }

        pthread_mutex_destroy(&hFifo->mutex);

        free(hFifo);
    }

    return ret;
}

/******************************************************************************
 * Fifo_get
 ******************************************************************************/
int Fifo_get(Fifo_Handle hFifo, char * ptrPtr)
{
    int flush;
    int numBytes;

    assert(hFifo);
    assert(ptrPtr);

    pthread_mutex_lock(&hFifo->mutex);
    flush = hFifo->flush;
    pthread_mutex_unlock(&hFifo->mutex);

    if (flush) {
        return Mod_EFLUSH;
    }

    numBytes = read(hFifo->pipes[0], ptrPtr, sizeof(char *));

    if (numBytes != sizeof(char *)) {
        pthread_mutex_lock(&hFifo->mutex);
        flush = hFifo->flush;
        if (flush) {
            hFifo->flush = FALSE;
        }
        pthread_mutex_unlock(&hFifo->mutex);

        if (flush) {
            return Mod_EFLUSH;
        }
        return Mod_EIO;
    }

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->numBufs--;
    pthread_mutex_unlock(&hFifo->mutex);

    return Mod_EOK;
}

int Fifo_get2(Fifo_Handle hFifo, char * ptrPtr, int size)
{
    int flush;
    int numBytes;

    assert(hFifo);
    assert(ptrPtr);

    pthread_mutex_lock(&hFifo->mutex);
    flush = hFifo->flush;
    pthread_mutex_unlock(&hFifo->mutex);

    if (flush) {
        return Mod_EFLUSH;
    }

    numBytes = read(hFifo->pipes[0], ptrPtr, size);

    if (numBytes != size) {
        pthread_mutex_lock(&hFifo->mutex);
        flush = hFifo->flush;
        if (flush) {
            hFifo->flush = FALSE;
        }
        pthread_mutex_unlock(&hFifo->mutex);

        if (flush) {
            return Mod_EFLUSH;
        }
        return Mod_EIO;
    }

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->numBufs-=size;
    pthread_mutex_unlock(&hFifo->mutex);

    return Mod_EOK;
}

/******************************************************************************
 * Fifo_flush
 ******************************************************************************/
int Fifo_flush(Fifo_Handle hFifo)
{
    char ch = 0xff;

    assert(hFifo);

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->flush = TRUE;
    pthread_mutex_unlock(&hFifo->mutex);

    /* Make sure any Fifo_get() calls are unblocked */
    if (write(hFifo->pipes[1], &ch, 1) != 1) {
        return Mod_EIO;
    }

    return Mod_EOK;
}

/******************************************************************************
 * Fifo_put
 ******************************************************************************/
int Fifo_put(Fifo_Handle hFifo, char *  ptr)
{
    assert(hFifo);
    assert(ptr);

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->numBufs++;
    pthread_mutex_unlock(&hFifo->mutex);

    if (write(hFifo->pipes[1], &ptr, sizeof(char *)) != sizeof(char *)) {
        return Mod_EIO;
    }

    return Mod_EOK;
}

int Fifo_put2(Fifo_Handle hFifo, char *  ptr, int size)
{
    assert(hFifo);
    assert(ptr);

    if((hFifo->numBufs+size) > hFifo->maxsize)
        return Mod_EINVAL;
    pthread_mutex_lock(&hFifo->mutex);
    hFifo->numBufs+=size;
    pthread_mutex_unlock(&hFifo->mutex);

    if (write(hFifo->pipes[1], ptr, size) != size) {
        return Mod_EIO;
    }

    return Mod_EOK;
}

/******************************************************************************
 * Fifo_getNumEntries
 ******************************************************************************/
int Fifo_getNumEntries(Fifo_Handle hFifo)
{
    int numEntries;

    assert(hFifo);

    pthread_mutex_lock(&hFifo->mutex);
    numEntries = hFifo->numBufs;
    pthread_mutex_unlock(&hFifo->mutex);

    return numEntries;
}


static int _set_default_xchannel_args(Xchannel_Args *args)
{
    if(args == NULL){
        ERR(" input params error\n");
        return -1;
    }

    memset(args, 0, sizeof(Xchannel_Args));


    args->vbit_rate = 100000; // 100k
    args->gop       = 50;

    args->abit_rate = 48000;
    args->o_channels= 2;
    args->o_sample_rate = 44100;

    //strcpy(args->stream_out_path, "rtmp://10.33.2.181/live/xctestzx01");
    //strcpy(args->stream_save_path, "/tmp/xcsave.ts");

    
    return 0;
}

static int _check_iargs_and_update_args(Xchannel_Args * xargs, Xchannel_Args * iargs, int * err)
{
    // don't check args pointer, trust it

    xargs->have_audio_stream = iargs->have_audio_stream;
    xargs->have_video_stream = iargs->have_video_stream;
    xargs->have_file_stream  = iargs->have_file_stream;
    xargs->have_rtmp_stream  = iargs->have_rtmp_stream;


    if(xargs->have_audio_stream <= 0 && xargs->have_video_stream <= 0){
        if(err)
            *err = XCHANNEL_ERROR_CODE_NO_INPUT;
        return -1;
    }

    if(xargs->have_file_stream <= 0 && xargs->have_rtmp_stream <= 0){
        if(err)
            *err = XCHANNEL_ERROR_CODE_NO_OUTPUT;
        return -1;
    }

    // check raw input params
    if(xargs->have_video_stream){
        if(VAL_IF_IN_RANGE(iargs->i_width, 176, 1920)){
            xargs->i_width = iargs->i_width; 
        }else{
            if(err)
                *err = XCHANNEL_ERROR_CODE_VIDEO_WIDTH_ERROR;
            return -1;
        }
        
        if(VAL_IF_IN_RANGE(iargs->i_height, 144, 1080)){
            xargs->i_height = iargs->i_height; 
        }else{
            if(err)
                *err = XCHANNEL_ERROR_CODE_VIDEO_HEIGHT_ERROR;
            return -1;
        }   

        xargs->o_pix_fmt = xargs->i_pix_fmt = AV_PIX_FMT_YUV420P; // now input raw video only support this fmt
            
        if(VAL_IF_IN_RANGE(iargs->i_frame_rate, 1, 30)){
            xargs->i_frame_rate = iargs->i_frame_rate; 
        }else{
            if(err)
                *err = XCHANNEL_ERROR_CODE_VIDEO_FRAME_RATE_ERROR;
            return -1;
        }
        
        if(VAL_IF_IN_RANGE(iargs->coder_type, VIDEO_CODER_TYPE_CAVLC, VIDEO_CODER_TYPE_CABAC)){
            xargs->coder_type = iargs->coder_type; 
        }else{
            if(err)
                *err = XCHANNEL_ERROR_CODE_VIDEO_FRAME_RATE_ERROR;
            return -1;
        }

    }

    if(xargs->have_audio_stream){
        if(VAL_IF_IN_RANGE(iargs->i_channels, 1, 2)){
            xargs->i_channels = iargs->i_channels; 
        }else{
            if(err)
                *err = XCHANNEL_ERROR_CODE_AUDIO_CHANNEL_ERROR;
            return -1;
        }
        
        if(iargs->i_sample_rate == 44100 || iargs->i_sample_rate == 22050){
            xargs->i_sample_rate = iargs->i_sample_rate; 
        }else{
            if(err)
                *err = XCHANNEL_ERROR_CODE_AUDIO_SAMPLE_RATE_ERROR;
            return -1;
        }

        if(VAL_IF_IN_RANGE(iargs->i_sample_fmt, XCHANNEL_AUDIO_SAMPLE_FORMAT_TYPE_U8, XCHANNEL_AUDIO_SAMPLE_FORMAT_TYPE_S16)){
            xargs->i_sample_fmt = iargs->i_sample_fmt; 
        }else{
            //printf("iargs->i_sample_fmt:%d\n", iargs->i_sample_fmt);
            if(err)
                *err = XCHANNEL_ERROR_CODE_AUDIO_SAMPLE_FMT_ERROR;
            return -1;
        }

    }






    // check encode params
    if(VAL_IF_IN_RANGE(iargs->vbit_rate, 100000, 1000000)){
        xargs->vbit_rate = iargs->vbit_rate; // 100k
    }

    if(VAL_IF_IN_RANGE(iargs->o_width, 176, 1920)){
        xargs->o_width = iargs->o_width; 
    }else{
        xargs->o_width = iargs->i_width;
    }

    if(VAL_IF_IN_RANGE(iargs->o_height, 144, 1080)){
        xargs->o_height = iargs->o_height; 
    }else{
        xargs->o_height = iargs->i_height;
    }

    if(VAL_IF_IN_RANGE(iargs->o_frame_rate, 1, 30)){
        xargs->o_frame_rate = iargs->o_frame_rate; 
    }else{
        xargs->o_frame_rate = iargs->i_frame_rate;
    }

    if(VAL_IF_IN_RANGE(iargs->gop, 25, 250)){
        xargs->gop = iargs->gop; 
    }

    if(VAL_IF_IN_RANGE(iargs->abit_rate, 32, 196)){
        xargs->abit_rate = iargs->abit_rate; 
    }

    if(VAL_IF_IN_RANGE(iargs->o_channels, 2, 2)){ // aac lib only support 2
        xargs->o_channels = iargs->o_channels; 
    }else{
        //xargs->o_channels = iargs->i_channels;
        xargs->o_channels = 2;
    }

    if(iargs->o_sample_rate == 44100 || iargs->o_sample_rate == 22050){
        //xargs->o_sample_rate = iargs->o_sample_rate; 
        xargs->o_sample_rate = 22050;
    }else{
        //xargs->o_sample_rate = iargs->i_sample_rate;
        xargs->o_sample_rate = 22050;
    }

    if(strlen(iargs->stream_out_path))
        strcpy(xargs->stream_out_path, iargs->stream_out_path);

    if(strlen(iargs->stream_save_path))
        strcpy(xargs->stream_save_path, iargs->stream_save_path);


    if(iargs->aextradata_size > 0){
        xargs->aextradata_size = iargs->aextradata_size;
        memcpy(xargs->aextradata, iargs->aextradata, iargs->aextradata_size);
    }else{
        xargs->aextradata[0] = 0x13;
        xargs->aextradata[1] = 0x90;
        xargs->aextradata_size = 2;
    }

    if(iargs->vextradata_size > 0){
        xargs->vextradata_size = iargs->vextradata_size;
        memcpy(xargs->vextradata, iargs->vextradata, iargs->vextradata_size);
    }
    return 0;
}

/*
static int _set_buffer_by_args(tvie_buf_Handle buffer, Xchannel_Args *xargs)
{
    // not check, please sure input params is ok

    buffer->have_audio_stream = 0;
    buffer->have_video_stream = 0; 

    if(xargs->have_video_stream > 0){
        buffer->have_video_stream = 1;
        buffer->width      = xargs->o_width;
        buffer->height     = xargs->o_height;
        buffer->frame_rate = xargs->o_frame_rate;
        buffer->pix_fmt    = AV_PIX_FMT_YUV420P; // default
        buffer->vcodec_id  = AV_CODEC_ID_H264; // default

        buffer->vextradata_size = xargs->vextradata_size;
        if(xargs->vextradata_size > 0)
            memcpy(buffer->vextradata, xargs->vextradata, xargs->vextradata_size);

    }

    if(xargs->have_audio_stream > 0){
        buffer->have_audio_stream = 1;
        //buffer->channels    = xargs->o_channels;
        buffer->channels    = 2; // now used aac lib need 2
        buffer->sample_rate = xargs->o_sample_rate;
        //buffer->sample_rate = 22050;
        buffer->sample_fmt  = AV_SAMPLE_FMT_S16; // default
        
        buffer->acodec_id = AV_CODEC_ID_AAC; // default
        //buffer->frame_size = 1024; // default
        buffer->channel_layout = AV_CH_LAYOUT_STEREO;

        buffer->aextradata_size = xargs->aextradata_size;
        if(xargs->aextradata_size > 0)
            memcpy(buffer->aextradata, xargs->aextradata, xargs->aextradata_size);

        
    }

    buffer->timebase = XCHANNEL_TIME_BASE;

    return 0;
}
*/

static void _set_video_params_to_ctx(AVCodecContext *c, Xchannel_Args * params, AVDictionary **opts)
{
    // common settings
    c->width = params->o_width;
    c->height = params->o_height;
    c->pix_fmt = params->o_pix_fmt;
    c->qmin = 0;
    c->qmax = 40;
    c->max_qdiff = 4; // default 4
    c->me_method = ME_HEX;
    c->me_range = 16;
    c->thread_count = 0; // auto
    
    // time base set, need fix in furture
    c->time_base.den = params->o_frame_rate;
    c->time_base.num = 1;

    // gop set
    c->gop_size = params->gop; /* emit one intra frame every twelve frames at most */  
    c->keyint_min = 25;
    
    // rate control settings
    c->bit_rate = params->vbit_rate;

    c->i_quant_factor = 1.4;
    c->b_quant_factor = 1.3;
    c->chromaoffset = 0;
    c->scenechange_threshold = 40;

    // if interlaced
    //c->flags = c->flags & CODEC_FLAG_INTERLACED_DCT;

    c->qblur = 0.5; 
    c->qcompress = 0.5; /* 0.0 => cbr, 1.0 => constant qp */
    c->max_b_frames = 2;
    c->refs = 3;
    c->trellis = 1;

    //c->noise_reduction = 0; //x264 default not init
    c->me_subpel_quality = 7;
    c->b_frame_strategy = 1;
    //c->me_cmp |= FF_CMP_CHROMA;
    //c->flags |= CODEC_FLAG_PSNR;

    
 
    c->rc_max_rate = c->bit_rate * 1.5;
    c->rc_min_rate = c->bit_rate * 0.8;  //not used by x264
    c->rc_buffer_size = c->bit_rate * 2;

    
    c->qmax = 36;

    //c->bit_rate_tolerance = c->bit_rate/4;
    c->rc_initial_buffer_occupancy = c->rc_buffer_size*9/10;
    //c->rc_buffer_aggressivity= (float)1.0;
    //c->rc_initial_cplx= 0.9;


    
    
    //c->flags |= CODEC_FLAG_LOOP_FILTER;


    // not do, because encode video not used. do it in feature.
    //set_video_params_profile(c, params, opts);
    //set_video_params_templates(c, params, opts);
    
    // set coder type
    switch(params->coder_type){
        case VIDEO_CODER_TYPE_CAVLC:
            c->coder_type = FF_CODER_TYPE_VLC;
            break;
        case VIDEO_CODER_TYPE_CABAC:
            c->coder_type = FF_CODER_TYPE_AC;
            break;
        default:
            c->coder_type = FF_CODER_TYPE_VLC;
            break;
    }

    // set zero delay
    av_dict_set(opts, "tune", "zerolatency", 0);


    if (c->codec_id == AV_CODEC_ID_H264){
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;    //dump sequence header in extradata
        c->flags |= CODEC_FLAG_CLOSED_GOP;
    }

}


static void _set_audio_params_to_ctx(AVCodecContext *c, Xchannel_Args * params)
{
    c->codec_type = AVMEDIA_TYPE_AUDIO;

    c->bit_rate = params->abit_rate;
    c->sample_rate = params->o_sample_rate;
    c->channels = params->o_channels;
    c->sample_fmt = params->i_sample_fmt;
    
    c->time_base.den = c->sample_rate;
    c->time_base.num = 1;
    //c->bit_rate_tolerance = 2 * 1000 * 1000;   //2Mbps
    c->bit_rate_tolerance = 2 * params->abit_rate;
    c->channel_layout = AV_CH_LAYOUT_STEREO;
    
    if (c->codec_id == AV_CODEC_ID_AAC){
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;    //dump sequence header in extradata
    }

}


static int _create_video_codec(Xchannel_Handle handle, int * err)
{
    // not check , trust it
    AVCodec *codec = NULL;
    AVDictionary *opts = NULL;
    int ret = 0;
    
    Xchannel_Args * xargs = &handle->args;
    int codec_id = AV_CODEC_ID_H264;

    // codec
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(codec == NULL){
        if(err)
            *err = XCHANNEL_ERROR_CODE_VIDEO_CODEC_ERROR;
        return -1;
    }

    //init video ctx
    handle->vctx = avcodec_alloc_context3(codec);
    if (handle->vctx == NULL) {
        if(err)
            *err = XCHANNEL_ERROR_CODE_VIDEO_CTX_ERROR;
        return -1;
    }

    //set video params
    handle->vctx->codec_id = codec_id;
    _set_video_params_to_ctx(handle->vctx, xargs, &opts);

    // open codec 
    ret = avcodec_open2(handle->vctx, codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        if(err)
            *err = XCHANNEL_ERROR_CODE_VIDEO_CODEC_OPEN_ERROR;

        return -1;
    }


    
    return 0;
}

static int _create_audio_codec(Xchannel_Handle handle, int * err)
{
    // not check , trust it
    AVCodec *codec = NULL;
    int ret = 0;
    
    Xchannel_Args * xargs = &handle->args;
    int codec_id = AV_CODEC_ID_AAC;

    // codec
    codec = avcodec_find_encoder_by_name("libfaac");
    if(codec == NULL){
        if(err)
            *err = XCHANNEL_ERROR_CODE_AUDIO_CODEC_ERROR;
        return -1;
    }

    //init  ctx
    handle->actx = avcodec_alloc_context3(codec);
    if (handle->actx == NULL) {
        if(err)
            *err = XCHANNEL_ERROR_CODE_AUDIO_CTX_ERROR;
        return -1;
    }

    //set params
    handle->actx->codec_id = codec_id;
    _set_audio_params_to_ctx(handle->actx, xargs);

    // open codec 
    ret = avcodec_open2(handle->actx, codec, NULL);
    if (ret < 0) {
        if(err)
            *err = XCHANNEL_ERROR_CODE_AUDIO_CODEC_OPEN_ERROR;

        return -1;
    }
    
    return 0;
}


static int _close_codecs(struct xchannel_obj * handle)
{
    if(handle == NULL){
        return XCHANNEL_ERROR_CODE_NULL_POINTER;
    }

    int ret;
    
    // close video codecs  
    if(handle->vctx){
        ret = avcodec_close(handle->vctx);
        av_free(handle->vctx);
    }
  
    // close audio codecs   
    if(handle->actx){
        ret = avcodec_close(handle->actx);
        av_free(handle->actx);
    }
    
    return 0;

}

static AVStream *_av_new_stream_my(AVFormatContext *s, int id)
{
    AVStream *st = avformat_new_stream(s, NULL);
    if (st)
        st->id = id;
    return st;
}


/* add a video output stream */
static AVStream * _add_video_stream(AVFormatContext *oc, enum CodecID codec_id,
                                Xchannel_Args * args)
{
    AVCodecContext *c;
    AVStream *st;

    st = _av_new_stream_my(oc, oc->nb_streams);
    //st = av_new_stream(oc, oc->nb_streams); 
    if (!st) {
        ERR("Could not alloc stream\n");
        exit(1);
    }
    //avcodec_get_context_defaults2(st->codec, AVMEDIA_TYPE_VIDEO);
    avcodec_get_context_defaults3(st->codec, NULL);

    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = args->vbit_rate;
    /* resolution must be a multiple of two */
    c->width = args->o_width;
    c->height = args->o_height;
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
    c->time_base.den = args->o_frame_rate;
    c->time_base.num = 1;
    c->gop_size = args->gop; /* emit one intra frame every twelve frames at most */

    c->max_b_frames = 2;
    c->pix_fmt = args->i_pix_fmt;
    
    //st->stream_copy = 1;
    
    // some formats want stream headers to be separate
    if(oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    c->thread_count = 1;

    //avcodec_thread_init(st->codec, c->thread_count);


    return st;
}

static AVStream *_add_audio_stream(AVFormatContext *oc, enum CodecID codec_id,
                                  Xchannel_Args * args)
{
    AVCodecContext *c;
    AVStream *st;

    st = _av_new_stream_my(oc, oc->nb_streams);
    //st = av_new_stream(oc, oc->nb_streams); 
    if (!st) {
        ERR("Could not alloc stream\n");
        exit(1);
    }
    //avcodec_get_context_defaults2(st->codec, CODEC_TYPE_AUDIO);
    avcodec_get_context_defaults3(st->codec, NULL);

    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_AUDIO; 


    /* put sample parameters */
    c->sample_fmt = AV_SAMPLE_FMT_S16;
    c->bit_rate = args->abit_rate;
    c->sample_rate = args->o_sample_rate;
    c->channels = 2;
    c->time_base.num = 1;
    c->time_base.den = c->sample_rate;
    
 ///////////////
    //st->stream_copy = 1;

    // some formats want stream headers to be separate
    if(oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

static int _set_stream_params(AVFormatContext *oc, int idx, struct xchannel_args * args)
{
    
    AVStream* outStream      = oc->streams[idx];
    AVCodecContext* outCtx   = outStream->codec;
    int extraSize;
    //int offset;
    
    if(outCtx->codec_type == AVMEDIA_TYPE_VIDEO){
        outCtx->codec_id = AV_CODEC_ID_H264;
        DEB3("vcodec_id : %d\n", outCtx->codec_id);
    }else if(outCtx->codec_type == AVMEDIA_TYPE_AUDIO){
        outCtx->codec_id = AV_CODEC_ID_AAC;
        DEB3("acodec_id : %d\n", outCtx->codec_id);
    }


    
    if(outCtx->codec_type == AVMEDIA_TYPE_AUDIO){
        
        outCtx->bit_rate = args->abit_rate;
        outCtx->sample_rate = args->o_sample_rate;
        outCtx->channels = args->o_channels;
        outCtx->sample_fmt = args->i_sample_fmt;
        
        outCtx->time_base.den = args->o_sample_rate;
        outCtx->time_base.num = 1;
        //c->bit_rate_tolerance = 2 * 1000 * 1000;   //2Mbps
        outCtx->bit_rate_tolerance = 2 * args->abit_rate;
        outCtx->channel_layout = AV_CH_LAYOUT_STEREO;
        
        if (outCtx->codec_id == AV_CODEC_ID_AAC){
            outCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;    //dump sequence header in extradata
        }

        if(outCtx->block_align == 1 && outCtx->codec_id == AV_CODEC_ID_MP3){
            outCtx->block_align = 0;
        }
        if(outCtx->codec_id == AV_CODEC_ID_AC3){
            outCtx->block_align = 0;
        }

        if(args->aextradata_size > 0){
             extraSize = args->aextradata_size + FF_INPUT_BUFFER_PADDING_SIZE;
             outCtx->extradata  = (uint8_t*)av_mallocz(extraSize); // this can be freed by avformat_free_context
             memcpy(outCtx->extradata, args->aextradata, args->aextradata_size);
             outCtx->extradata_size = args->aextradata_size;
        }
    }

    if(outCtx->codec_type == AVMEDIA_TYPE_VIDEO){
        
        outCtx->pix_fmt = args->o_pix_fmt;
        outCtx->width   = args->o_width;
        outCtx->height  = args->o_height;
        outCtx->flags   = args->o_height;

        outCtx->time_base.den    = args->o_frame_rate;
        outCtx->time_base.num    = 1;

        outCtx->bit_rate = args->vbit_rate;
        
        if(args->vextradata_size > 0){
             extraSize = args->vextradata_size + FF_INPUT_BUFFER_PADDING_SIZE;
             outCtx->extradata  = (uint8_t*)av_mallocz(extraSize); // this can be freed by avformat_free_context
             outCtx->extradata_size = args->vextradata_size;
             memcpy(outCtx->extradata, args->vextradata, outCtx->extradata_size);
        }

    }
    
    if(oc->oformat->flags & AVFMT_GLOBALHEADER)
    {
        outCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }


}


static int _xchannel_interrupt_cb(void *h)
{
    Xchannel_Handle handle = (Xchannel_Handle)h;
    int64_t now = 0;

    now = av_gettime();

    DEB1("now(%lld) - start(%lld) = %lld\n", now, handle->start_time, now - handle->start_time );
   
    if(now - handle->start_time > 5000000){
        ERR("_xchannel_interrupt_cb return.\n");
        return 1;
    }

    return 0;
}


static int _create_output_ctx(Xchannel_Handle handle, char *format, int type)
{
    AVOutputFormat *fmt = NULL;
    AVFormatContext *oc = NULL;
    AVStream *audio_st, *video_st;
    int video_stream_idx, audio_stream_idx;
    int i;

    char * path = NULL;
    AVDictionary *opts = NULL;

    if(type == 0){
        // rtmp
        path = handle->args.stream_out_path;
    }else{
        path = handle->args.stream_save_path;

    }
    
    fmt = av_guess_format(format, NULL, NULL);
    if (!fmt) {
        return -1;
    }

    /* allocate the output media context */
    oc = avformat_alloc_context();
    if (!oc) {
        return -1;
    }
    oc->oformat = fmt;
    //oc->max_delay = 500000;
    //oc->flags |= AVFMT_FLAG_NONBLOCK;

    oc->interrupt_callback.callback = _xchannel_interrupt_cb;
    oc->interrupt_callback.opaque = handle;

    strcpy(oc->filename, path); 

    handle->start_time = av_gettime();

    av_dict_set(&opts, "timeout", "5", 0); // in secs

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        //if (avio_open(&oc->pb, path, AVIO_FLAG_WRITE|AVIO_FLAG_NONBLOCK) < 0) {
        //    ERR("Could not open '%s'\n", oc->filename);
        //    free(oc);  // need free 
        //    return -1;
        //}
        if (avio_open2(&oc->pb, path, AVIO_FLAG_WRITE, &oc->interrupt_callback, NULL) < 0) {
            ERR("Could not open '%s'\n", oc->filename);
            free(oc);  // need free 
            return -1;
        } 
    }else{
        DEB1("url '%s' no need to open\n", oc->filename);
    }

    /* add the audio and video streams using the default format codecs
       and initialize the codecs */
    video_st = NULL;
    audio_st = NULL;
    if ((fmt->video_codec != AV_CODEC_ID_NONE) && handle->args.have_video_stream) {
        video_st = _add_video_stream(oc, AV_CODEC_ID_H264, &handle->args);
    }
    
    if ((fmt->audio_codec != AV_CODEC_ID_NONE) && handle->args.have_audio_stream) {
        audio_st = _add_audio_stream(oc, AV_CODEC_ID_AAC, &handle->args);
    }

    av_dump_format(oc, 0, oc->filename, 1);

    // Find audio/video stream.
    video_stream_idx = audio_stream_idx = -1;
    for (i = 0; i < oc->nb_streams; i++) {
        if (video_stream_idx == -1 && oc->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            _set_stream_params(oc, i, &handle->args);
        }
        if (audio_stream_idx == -1 && oc->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            _set_stream_params(oc, i, &handle->args);
        }
    }

    /* write the stream header, if any */
    if(avformat_write_header(oc, NULL)){
        ERR("av_write_header error!\n");
    }

    if(type == 0){
        handle->outctx = oc;
        handle->out_video_stream_index = video_stream_idx;
        handle->out_audio_stream_index = audio_stream_idx;
    }else{
        handle->savectx = oc;
        handle->save_video_stream_index = video_stream_idx;
        handle->save_audio_stream_index = audio_stream_idx;

    }

    return 0;

}

/*
returns: 
    NULL(false, and err been set error code)
    non NULL(ok)
*/
Xchannel_Handle xchannel_open(Xchannel_Args *iargs, int * err)
{
    Xchannel_Args * xargs = NULL;

    if(err == NULL){
        return NULL;
    }

    // check input args, if error set the error code and return NULL.
    if(iargs == NULL){
        if(err != NULL){
            *err = XCHANNEL_ERROR_CODE_INPUT_ARGS_NULL;
        }
        return NULL;
    }
    
    // malloc a space
    Xchannel_Handle handle = (Xchannel_Handle)malloc(sizeof(struct xchannel_obj));
    if(handle == NULL){
        if(err != NULL){
            *err = XCHANNEL_ERROR_CODE_MEMORY_ALLOC_ERROR;
        }
        return NULL;
    }

    // init handle
    memset(handle, 0, sizeof(struct xchannel_obj));

    xargs = &handle->args;
    // set default args
    _set_default_xchannel_args(xargs);
    
    // check iargs and update to xargs
    if(_check_iargs_and_update_args(xargs, iargs, err) < 0){
        free(handle);
        return NULL;
    }

    /* register all the codecs */
    avcodec_register_all();
    av_register_all();
    avformat_network_init();


    // create codecs
    if(xargs->have_video_stream){
        if(_create_video_codec(handle, err) < 0){
            goto xchannel_open_err;

        }
    }

    if(xargs->have_audio_stream){
        if(_create_audio_codec(handle, err) < 0){
            goto xchannel_open_err;
        }
    }


    // create output
    handle->out_audio_stream_index = -1;
    handle->out_video_stream_index = -1;
    handle->save_audio_stream_index = -1;
    handle->save_video_stream_index = -1;
    if(xargs->have_rtmp_stream){
        if(_create_output_ctx(handle, "flv", 0)){
            *err = XCHANNEL_ERROR_CODE_OUT_CTX_CREATE_ERROR;
            goto xchannel_open_err;
        }
    }
    if(xargs->have_file_stream){
        if(_create_output_ctx(handle, "flv", 1)){
            *err = XCHANNEL_ERROR_CODE_OUT_CTX_CREATE_ERROR;
            goto xchannel_open_err;
        }
    }


    if(err){
        *err = XCHANNEL_ERROR_CODE_OK;
    }
    
    return handle;

    
    xchannel_open_err:
    // clear
    if(handle != NULL){
        _close_codecs(handle);
        free(handle);
    }

    return NULL;
    
}



static int _pack_raw_video_pkt(struct xchannel_obj * handle, Xchannel_Data * data, AVPacket ** out)
{

}

static int _pack_raw_audio_pkt(struct xchannel_obj * handle, Xchannel_Data * data, AVPacket ** out)
{
    AVFrame *sample = NULL;  // for audio data
    Xchannel_Args * xargs = NULL;
    char * sample_buf = NULL;
    int sample_size;
    int ret;
    int out_size;
    AVPacket *pkt;
    int got_output;

    
    if(handle == NULL || data == NULL){
        return XCHANNEL_ERROR_CODE_NULL_POINTER;
    }

    *out = NULL;

    xargs = &handle->args;
    // malloc
    sample = avcodec_alloc_frame();
    if(sample == NULL){
        ERR("sample avcodec_alloc_frame failed.\n");
        return XCHANNEL_ERROR_CODE_MALLOC_ERROR;
    }
    
    // filling data
    if(xargs->i_channels == 2){
        sample_size = data->size;
        sample_buf = malloc(sample_size);
        memcpy(sample_buf, data->data, data->size);
    }else if(xargs->i_channels == 1){
    
        sample_size = data->size;
        sample_buf = malloc(sample_size);
        memcpy(sample_buf, data->data, data->size);

    }else{
        ERR("i_channels %d not support.\n", xargs->i_channels);
        return XCHANNEL_ERROR_CODE_AUDIO_CHANNEL_ERROR;
    }
    
    sample->nb_samples = data->size >> 2; // when 2 channel
    DEB3("nb_samples:%d, frame_size:%d\n", sample->nb_samples, handle->actx->frame_size);
    
    if ((ret = avcodec_fill_audio_frame(sample, 2, AV_SAMPLE_FMT_S16,
                  (const uint8_t *)sample_buf, sample_size, 0)) < 0){
        return XCHANNEL_ERROR_CODE_AUDIO_DATA_FILL_ERROR;
    }


    pkt = av_malloc(sizeof(AVPacket));
    if (pkt == NULL) {
        return XCHANNEL_ERROR_CODE_MALLOC_ERROR;
    }

    av_init_packet(pkt);
    pkt->destruct = av_destruct_packet;
    pkt->data = NULL; // packet data will be allocated by the encoder
    pkt->size = 0;

    // encode the samples 
    ret = avcodec_encode_audio2(handle->actx, pkt, sample, &got_output);
    if (ret < 0) {
        return XCHANNEL_ERROR_CODE_AUDIO_ENCODE_ERROR;
    }
    DEB3("AAC header: size=%d, 0x%x, 0x%x \n", 
        handle->actx->extradata_size, handle->actx->extradata[0], handle->actx->extradata[1]);


    // free the audio raw data
    if(sample->data[0])
        free(sample->data[0]);
    avcodec_free_frame(&sample);
    
    if(got_output){
        // rebuild timestamp
        data->pts &= 0xffffffffffff;
        pkt->pts = pkt->dts = av_rescale(data->pts, XCHANNEL_TIME_BASE, data->timebase);
        //pkt->pts = pkt->dts = data->pts/10000;

        // the key frame if need be set 
        if (data->is_key) {
            pkt->flags |= AV_PKT_FLAG_KEY;
        }

        if(handle->out_audio_stream_index >= 0)
            pkt->stream_index = handle->out_audio_stream_index;
        else
            pkt->stream_index = handle->save_audio_stream_index;

        *out = pkt;
    }else{
        av_free_packet(pkt);
        av_free(pkt);

    }

    // aac need header
    //if(handle->args.aextradata_size == 0){
    //    handle->args.aextradata_size = ;
   // }

    return 0;
    
}

AVPacket * _pack_enc_video_pkt(struct xchannel_obj * handle, Xchannel_Data * data)
{
    if(data == NULL)
        return NULL;

    AVPacket *pkt = NULL;
    
    // malloc a pkt to fill encoded video
    pkt = malloc(sizeof(AVPacket));
    if (pkt == NULL) {
        ERR("malloc failed for video pkt\n");
        return NULL;
    }
    av_init_packet(pkt);
    pkt->destruct = av_destruct_packet;
    pkt->data     = av_mallocz(data->size);
    if(pkt->data == NULL){
        ERR("malloc failed for pkt->data\n");
        return NULL;
    }
    pkt->size = data->size;
    
    // filling data
    memcpy(pkt->data, data->data, data->size);
    
    // the key frame if need be set 
    if (data->is_key) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }

    // rebuild timestamp
    data->pts &= 0xffffffffffff;
    data->dts &= 0xffffffffffff;
    pkt->pts = av_rescale(data->pts, XCHANNEL_TIME_BASE, data->timebase);
    pkt->dts = av_rescale(data->dts, XCHANNEL_TIME_BASE, data->timebase);


    // set stream index, need fix
    if(handle->out_video_stream_index >= 0)
        pkt->stream_index = handle->out_video_stream_index;
    else
        pkt->stream_index = handle->save_video_stream_index;

    
    return pkt;
    
}

AVPacket * _pack_enc_audio_pkt(struct xchannel_obj * handle, Xchannel_Data * data)
{
    if(data == NULL)
        return NULL;

    AVPacket *pkt = NULL;
    
    // malloc a pkt to fill encoded data
    pkt = malloc(sizeof(AVPacket));
    if (pkt == NULL) {
        ERR("malloc failed for audio pkt\n");
        return NULL;
    }
    av_init_packet(pkt);
    pkt->destruct = av_destruct_packet;
    pkt->data     = av_mallocz(data->size);
    if(pkt->data == NULL){
        ERR("malloc failed for pkt->data\n");
        return NULL;
    }
    pkt->size = data->size;
    
    // filling data
    memcpy(pkt->data, data->data, data->size);
    
    // the key frame if need be set 
    if (data->is_key) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }
    
    // rebuild timestamp
    pkt->pts = av_rescale(data->pts, XCHANNEL_TIME_BASE, data->timebase);
    pkt->dts = pkt->pts;
    
    // set stream index, need fix
    if(handle->out_audio_stream_index >= 0)
        pkt->stream_index = handle->out_audio_stream_index;
    else
        pkt->stream_index = handle->save_audio_stream_index;



    return pkt;
    
}


static int _rebuild_timestamp(struct xchannel_obj * handle, AVPacket *pkt)
{
    if(handle == NULL || pkt == NULL){
        return XCHANNEL_ERROR_CODE_NULL_POINTER;
    }
    int ret = 0;

    int timebase = 0;

    timebase = XCHANNEL_TIME_BASE;
    
    int64_t     dt;  // through framerate calculate dt
    int64_t     dt_pts_dts;

    //pkt->pts = pkt->pts & 0xffffffff;
    //pkt->dts = pkt->dts & 0xffffffff;
    
    // get the val = pts - dts, val will used to rebuild pts by rebuilt dts
    dt_pts_dts = pkt->pts - pkt->dts;

    if(handle->args.o_frame_rate){
        dt = 2*(timebase/handle->args.o_frame_rate);
    }else{
        dt = 2*(timebase/25);
    }
    
    //printf("timebase:%d, dt:%lld.\n",timebase,dt);
    
    if(handle->args.have_video_stream > 0){
        
        if(pkt->stream_index == handle->out_video_stream_index ||
            pkt->stream_index == handle->save_video_stream_index){
        //video
            //printf("video,first_vdts:%lld, vdts:%lld, pts:%lld, dts:%lld.\n",handle->in_first_vdts,handle->vdts,pkt->pts,pkt->dts);
            if(handle->in_first_vdts <= 0){
                
                handle->in_first_vdts = pkt->dts;  // save the first dts
                handle->vdts = 0;

            }else{
                handle->vdts = pkt->dts - handle->in_first_vdts;
            }

            pkt->dts = handle->vdts;
            pkt->pts = pkt->dts + dt_pts_dts + dt;
            
        }
        
        if(pkt->stream_index == handle->out_audio_stream_index ||
            pkt->stream_index == handle->save_audio_stream_index){
        // audio
            if(handle->in_first_vdts <= 0){
                // if video dts not rebuild
                DEB3("handle->in_first_vdts <= 0\n");
                return -1;
            }

            if(handle->in_first_adts == 0){
                if(pkt->dts <= handle->in_first_vdts){
                    DEB3("audio dts > first vdts\n");
                    return -1;
                }else{
                    handle->in_first_adts = pkt->dts;
                    handle->adts = 0;
                }
            }else{
                handle->adts = pkt->dts - handle->in_first_adts;
                
            }

            pkt->dts = handle->adts;
            pkt->pts = pkt->dts + dt_pts_dts + dt;

        }
    }
    else{
        if(pkt->stream_index == handle->out_audio_stream_index ||
            pkt->stream_index == handle->save_audio_stream_index){

        //audio
            if(handle->in_first_adts == 0){
                handle->in_first_adts = pkt->dts;
                handle->adts = 0;
                
            }else{
                handle->adts = pkt->dts - handle->in_first_adts;
                
            }
            
            pkt->dts = handle->adts;
            pkt->pts = pkt->dts + dt_pts_dts + dt;

        }

    }
    return ret;
}



int xchannel_encode(struct xchannel_obj * handle, Xchannel_Data * data)
{

    if(handle == NULL || data == NULL){
        return XCHANNEL_ERROR_CODE_INPUT_ARGS_NULL;
    }

    int ret = 0;
    Xchannel_Args * xargs = &handle->args;  
    AVPacket * pkt = NULL;

    
    if(xargs == NULL)
        return XCHANNEL_ERROR_CODE_INPUT_ARGS_NULL;


    DEB3("data type:%d, size:%d, pts:%lld\n", data->type, data->size, data->pts);

    // check the data, and pack it 
    if(data->type == XCHANNEL_DATA_TYPE_ENC_VIDEO && handle->args.have_video_stream){
        
        pkt = _pack_enc_video_pkt(handle, data);
        if(pkt == NULL){
            DEB3("_pack_enc_video_pkt NULL\n");
            return XCHANNEL_ERROR_CODE_PACK_ENC_VIDEO_ERROR;
        }
    }
    else if(data->type == XCHANNEL_DATA_TYPE_RAW_AUDIO && handle->args.have_audio_stream){
        ret = _pack_raw_audio_pkt(handle, data, &pkt);
        if(ret < 0){
            DEB3("_pack_raw_audio_pkt error\n");
            return ret;
        }

    }
    else if(data->type == XCHANNEL_DATA_TYPE_RAW_VIDEO && handle->args.have_video_stream){
        return XCHANNEL_ERROR_CODE_NOT_SUPPORT_VIDEO_RAW_TYPE;
        // not support yeat, because phone encode video very slow.
        ret = _pack_raw_video_pkt(handle, data, &pkt);
        if(ret < 0){
            return ret;
        }

    }
    else if(data->type == XCHANNEL_DATA_TYPE_ENC_AUDIO && handle->args.have_audio_stream){
        pkt = _pack_enc_audio_pkt(handle, data);
        if(pkt == NULL){
            DEB3("_pack_enc_audio_pkt error\n");
            return XCHANNEL_ERROR_CODE_PACK_ENC_AUDIO_ERROR;
        }

    }
    else{
        DEB3("date type error\n");
        return XCHANNEL_ERROR_CODE_DATA_TYPE_ERROR;
    }


    // rebuild timestamp
    if(pkt){

        DEB3("save_v_index:%d, save_a_index:%d, out_v_index:%d, out_a_index:%d\n",
            handle->save_video_stream_index, handle->save_audio_stream_index,
            handle->out_video_stream_index, handle->out_audio_stream_index);

        DEB3("before rebuild, type: %d, timebase:%lld, index:%d, pts:%lld,dts:%lld\n", 
            data->type, data->timebase, pkt->stream_index, pkt->pts, pkt->dts);
                
        if(_rebuild_timestamp(handle, pkt) < 0){
        
            av_free_packet(pkt);
            av_free(pkt);
            DEB3("_rebuild_timestamp error\n");
            return ret;
        }

        DEB3("after rebuild, type: %d, timebase:%lld, index:%d, pts:%lld,dts:%lld\n", 
            data->type, data->timebase, pkt->stream_index, pkt->pts, pkt->dts);

    }else{
        DEB3("pkt NULL, type:%d, ts:%lld\n", data->type, data->pts);
    }
    // output stream

    handle->start_time = av_gettime();
    
    #if 1
    if(handle->outctx != NULL && pkt){
        DEB1("write a frame to out, type: %d, pts:%lld, dts:%lld, vcount:%lld, acont:%lld.\n", data->type,pkt->pts, pkt->dts,
            handle->outctx->streams[handle->out_video_stream_index]->nb_frames,
            handle->outctx->streams[handle->out_audio_stream_index]->nb_frames);
        if (av_interleaved_write_frame(handle->outctx, pkt) < 0) {
            ERR("error av_write_frame.\n");
            return XCHANNEL_ERROR_CODE_OUT_STREAM_ERROR;
        }

        handle->video_frame_cnt = handle->outctx->streams[handle->out_video_stream_index]->nb_frames;
        handle->audio_sample_cnt = handle->outctx->streams[handle->out_audio_stream_index]->nb_frames;
    }
        
    #endif
    
    // output stream
    #if 1
    if(handle->savectx != NULL && pkt){
        
        if (av_interleaved_write_frame(handle->savectx, pkt) < 0) {
            ERR("error av_write_frame.\n");
            return XCHANNEL_ERROR_CODE_OUT_FILE_ERROR;
        }
        DEB3("write a frame to file, type: %d, pts:%d.\n", 
            data->type,pkt->pts, pkt->dts);
    }
    #endif 

    av_free_packet(pkt);
    av_free(pkt);

    
    
    return ret;
}



static void _free_delayed_video_frames(struct xchannel_obj * handle)
{

}

static void _free_delayed_audio_frames(struct xchannel_obj * handle)
{
    int got_output;
    int ret;
    AVPacket pkt;

    // get the delayed frames 
    for (got_output = 1; got_output; ) {
        
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        
        ret = avcodec_encode_audio2(handle->actx, &pkt, NULL, &got_output);
        if (ret < 0) {
            return;
        }
        if (got_output) {
            av_free_packet(&pkt);
        }
    }

}


void xchannel_close(struct xchannel_obj * handle)
{
    // free the delayed frames 
    _free_delayed_audio_frames(handle);

    // close video ctx, close audio ctx
    _close_codecs(handle);

    // close out ctx
    if(handle->outctx)
        avformat_close_input(&handle->outctx);

    // close save ctx
    if(handle->savectx)
        avformat_close_input(&handle->savectx);

    
    free(handle);
}
