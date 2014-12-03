#ifndef _XCHANNEL_H_
#define _XCHANNEL_H_

#include <sys/types.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/common.h>



//#include <faac.h>

#define XC_PATH_LEN 1024
//#define XCHANNEL_TIME_BASE   1000000
#define XCHANNEL_TIME_BASE   1000
 


/*** structs ***/
enum xchannel_audio_sample_format_type{
    XCHANNEL_AUDIO_SAMPLE_FORMAT_TYPE_NONE = -1,
    XCHANNEL_AUDIO_SAMPLE_FORMAT_TYPE_U8 = 0, ///< unsigned 8 bits
    XCHANNEL_AUDIO_SAMPLE_FORMAT_TYPE_S16 = 1, ///< signed 16 bits
    XCHANNEL_AUDIO_SAMPLE_FORMAT_TYPE_S32 = 2, ///< signed 32 bits
};

// xchannel_error_code
#define     XCHANNEL_ERROR_CODE_OK  0

#define     XCHANNEL_ERROR_CODE_MALLOC_ERROR                (-10)
#define     XCHANNEL_ERROR_CODE_NULL_POINTER                (-11)


#define     XCHANNEL_ERROR_CODE_INPUT_ARGS_NULL             (-100)
#define     XCHANNEL_ERROR_CODE_MEMORY_ALLOC_ERROR          (-101)
#define     XCHANNEL_ERROR_CODE_NO_OUTPUT                   (-102)
#define     XCHANNEL_ERROR_CODE_NO_INPUT                    (-103)
#define     XCHANNEL_ERROR_CODE_DATA_TYPE_ERROR             (-104)
#define     XCHANNEL_ERROR_CODE_PACK_RAW_VIDEO_ERROR        (-105)
#define     XCHANNEL_ERROR_CODE_PACK_RAW_AUDIO_ERROR        (-106)
#define     XCHANNEL_ERROR_CODE_PACK_ENC_VIDEO_ERROR        (-107)
#define     XCHANNEL_ERROR_CODE_PACK_ENC_AUDIO_ERROR        (-108)
#define     XCHANNEL_ERROR_CODE_NOT_SUPPORT_VIDEO_RAW_TYPE  (-109)
#define     XCHANNEL_ERROR_CODE_NOT_SUPPORT_VIDEO_ENC_TYPE  (-110)
#define     XCHANNEL_ERROR_CODE_NOT_SUPPORT_AUDIO_RAW_TYPE  (-111)
#define     XCHANNEL_ERROR_CODE_NOT_SUPPORT_AUDIO_ENC_TYPE  (-112)



#define     XCHANNEL_ERROR_CODE_VIDEO_WIDTH_ERROR           (-200)
#define     XCHANNEL_ERROR_CODE_VIDEO_HEIGHT_ERROR          (-201)
#define     XCHANNEL_ERROR_CODE_VIDEO_FRAME_RATE_ERROR      (-202)
#define     XCHANNEL_ERROR_CODE_VIDEO_CODEC_ERROR           (-203)
#define     XCHANNEL_ERROR_CODE_VIDEO_CTX_ERROR             (-204)
#define     XCHANNEL_ERROR_CODE_VIDEO_CODEC_OPEN_ERROR      (-205)



#define     XCHANNEL_ERROR_CODE_AUDIO_CHANNEL_ERROR         (-300)
#define     XCHANNEL_ERROR_CODE_AUDIO_SAMPLE_RATE_ERROR     (-301)
#define     XCHANNEL_ERROR_CODE_AUDIO_SAMPLE_FMT_ERROR      (-302)
#define     XCHANNEL_ERROR_CODE_AUDIO_CODEC_ERROR           (-303)
#define     XCHANNEL_ERROR_CODE_AUDIO_CTX_ERROR             (-304)
#define     XCHANNEL_ERROR_CODE_AUDIO_CODEC_OPEN_ERROR      (-305)
#define     XCHANNEL_ERROR_CODE_AUDIO_DATA_FILL_ERROR       (-306)
#define     XCHANNEL_ERROR_CODE_AUDIO_ENCODE_ERROR          (-307)


#define     XCHANNEL_ERROR_CODE_OUT_CTX_CREATE_ERROR        (-400)
#define     XCHANNEL_ERROR_CODE_OUT_STREAM_ERROR            (-401)
#define     XCHANNEL_ERROR_CODE_OUT_FILE_ERROR              (-402)


#define Mod_EINVAL     -5   /**< Invalid input arguments (failure). */
#define Mod_ENOMEM     -4   /**< No memory available (failure). */
#define Mod_EIO        -3   /**< An IO error occured (failure). */
#define Mod_ENOTIMPL   -2   /**< Functionality not implemented (failure). */
#define Mod_EFAIL      -1   /**< General failure code (failure). */
#define Mod_EOK         0   /**< General success code (success). */
#define Mod_EFLUSH      1   /**< The command was flushed (success). */
#define Mod_EPRIME      2   /**< The command was primed (success). */
#define Mod_EFIRSTFIELD 3   /**< Only the first field was processed (success)*/
#define Mod_EBITERROR   4   /**< There was a non fatal bit error (success). */
#define Mod_ETIMEOUT    5   /**< The operation was timed out (success). */
#define Mod_EEOF        6   /**< The operation reached end of file */
#define Mod_EAGAIN      7   /**< The command needs to be rerun (success). */


#define FALSE 0
#define TRUE  1


enum xchannel_vidoe_coder_type{
    VIDEO_CODER_TYPE_CAVLC, 
    VIDEO_CODER_TYPE_CABAC, 
};




typedef struct Fifo_Object {
    pthread_mutex_t mutex;
    int             numBufs;
    int             flush;
    int             pipes[2];
    int             maxsize;
    int             inModId;
    int             outModId;
    int             inused;
    int             outused;
    int             switchflag;
} Fifo_Object;

/**
 * @brief       Handle through which to reference a Fifo.
 */
typedef struct Fifo_Object *Fifo_Handle;

/**
 * @brief       Attributes used to create a Fifo.
 * @see         Fifo_Attrs_DEFAULT.
 */
typedef struct Fifo_Attrs {
    /** 
     * @brief      Maximum elements that can be put on the Fifo at once
     * @remarks    For Bios only, Linux ignores this attribute
     */     
    int maxElems;
} Fifo_Attrs;







typedef struct xchannel_args{
    int have_video_stream; // "must be set !"
    int have_audio_stream; // "must be set !"
    int have_file_stream; // 
    int have_rtmp_stream; 

    // video raw data, the video true data params
    int i_width;   // range 176 to 1920, "must be set !"
    int i_height;  // range 144 to 1080, "must be set !"
    int i_pix_fmt; // now only support yv12(420), no need to set
    int i_frame_rate; // range 1 to 30, "must be set !"
    
    // audio raw data, the audio true data params
    int i_channels; // range 1 to 2, "must be set !"
    int i_sample_rate; // "must be set !", support 44100,22050
    int i_sample_fmt; // "must be set !", use enum xchannel_audio_sample_format_type  

    // video enc params, the params which you want to encode
    int vbit_rate;  // range 64000 to 1000000, default 100000bps
    int o_width;    // range 10 to 1920, out of range will be set same as input
    int o_height;   // range 10 to 1080, out of range will be set same as input
    int o_pix_fmt;  // now only support yv12(420), no need to set
    int o_frame_rate;  // range 1 to 30, out of range will be set same as input
    int gop;        // range 25 to 250, default 50
    int coder_type; // 
    
    // audio enc params, the params which you want to encode
    int abit_rate;   //sugest 48000, 64000, default 48000, range 32k to 196k
    int o_channels;  // range 1 to 2, out of range will be set same as input
    int o_sample_rate; // suggest 44100, out of range will be set same as input

    // others
    char stream_out_path[XC_PATH_LEN]; // "must be set !"
    char stream_save_path[XC_PATH_LEN]; // "must be set !"

    int  aextradata_size; 
    int  vextradata_size; // used for input data is encoded type
    char aextradata[128];
    char vextradata[256];

}Xchannel_Args;

enum xchannel_data_type{
    XCHANNEL_DATA_TYPE_NONE,
    XCHANNEL_DATA_TYPE_RAW_VIDEO, // raw video(yuv420)
    XCHANNEL_DATA_TYPE_RAW_AUDIO, // raw audio(pcm)
    XCHANNEL_DATA_TYPE_ENC_VIDEO,  // encoded video (h264 nals)
    XCHANNEL_DATA_TYPE_ENC_AUDIO,  // encoded audio
    XCHANNEL_DATA_TYPE_FILE,// file path
};


typedef struct xchannel_data{
    int type; //video, audio, use enum xchannel_data_type;
    int size; // data size
    int64_t timebase;
    int64_t pts;
    int64_t dts;
    void * data; // the pointer which point to data space
    int is_key;
}Xchannel_Data;


struct xchannel_obj{
    
    int video_frame_cnt;
    int audio_sample_cnt;
    
    // new 
    AVCodecContext *actx; // audio ctx for encode
    AVCodecContext *vctx; // video ctx for encode
    AVFormatContext *outctx;  // output ctx
    AVFormatContext *savectx;  // output ctx
    int out_video_stream_index;
    int out_audio_stream_index;
    int save_video_stream_index;
    int save_audio_stream_index;
    
    Xchannel_Args args;
    
    // for rebuild timestamp
    int64_t in_first_vdts;  // input first video dts
    int64_t vdts;           // rebuilt video dts
    int64_t in_first_adts;  // input first audio dts
    int64_t adts;

    int64_t start_time;
    
};
typedef struct xchannel_obj * Xchannel_Handle;




// functions
Xchannel_Handle xchannel_open(Xchannel_Args *, int * err);
int xchannel_encode(struct xchannel_obj * , Xchannel_Data *);
void xchannel_close(struct xchannel_obj *);


#endif
