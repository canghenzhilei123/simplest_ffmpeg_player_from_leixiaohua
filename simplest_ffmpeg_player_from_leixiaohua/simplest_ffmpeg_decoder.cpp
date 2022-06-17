/**
 * 最简单的基于FFmpeg的视频解码器
 * Simplest FFmpeg Decoder
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 *
 * 本程序实现了视频文件解码为YUV数据。它使用了libavcodec和
 * libavformat。是最简单的FFmpeg视频解码方面的教程。
 * 通过学习本例子可以了解FFmpeg的解码流程。
 * This software is a simplest decoder based on FFmpeg.
 * It decodes video to YUV pixel data.
 * It uses libavcodec and libavformat.
 * Suitable for beginner of FFmpeg.
 *
 * 2022.6.17，杨云鹏修改  VS2017上Relese x64编译通过
 * 1.ffmpeg版本修改为4.2.1
 * 2.删除废弃接口av_register_all
 * 3.修改已经被废弃的类型AVStream::codec
 * 4.修改已经被废弃的接口avcodec_decode_video2
 * 5.修改已经被废弃的接口av_free_packet
 * 
 */



#include <stdio.h>
#include <iostream>

#ifdef _WIN64
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif

int main(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx = NULL;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx = NULL;
	AVCodec			*pCodec = NULL;
	AVFrame	*pFrame = NULL,*pFrameYUV = NULL;
	unsigned char *out_buffer = NULL;
	AVPacket *packet = NULL;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx = NULL;

	const char *filepath="../../resource/Titanic.mkv";

	FILE *fp_yuv=fopen("output.yuv","wb+");  

	//av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
		//if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	
	if(videoindex==-1){
		printf("Didn't find a video stream.\n");
		return -1;
	}

	// pCodecCtx=pFormatCtx->streams[videoindex]->codec;

	pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext \n");
		return -1;
	}
	
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);
	if (pFormatCtx->streams[videoindex]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
		printf("Couldn't find audio stream information \n");
		return -1;
	}

	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		printf("Codec not found.\n");
		return -1;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}
	
	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  pCodecCtx->width, pCodecCtx->height,1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize,out_buffer,
		AV_PIX_FMT_YUV420P,pCodecCtx->width, pCodecCtx->height,1);

	
	
	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);
	printf("-------------------------------------------------\n");
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 

	while(av_read_frame(pFormatCtx, packet)>=0){
		if(packet->stream_index==videoindex){
			// ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			ret = avcodec_send_frame(pCodecCtx, pFrame);
			if(ret < 0){
				printf("Decode Error.\n");
				return -1;
			}
			got_picture = avcodec_receive_packet(pCodecCtx, packet);
			if(got_picture){
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
					pFrameYUV->data, pFrameYUV->linesize);

				y_size=pCodecCtx->width*pCodecCtx->height;  
				fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
				fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
				printf("Succeed to decode 1 frame!\n");

			}
		}
		//av_free_packet(packet);
		av_packet_unref(packet);
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (1) {
		//ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		ret = avcodec_send_frame(pCodecCtx, pFrame);
		if (ret < 0)
			break;
		got_picture = avcodec_receive_packet(pCodecCtx, packet);
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
			pFrameYUV->data, pFrameYUV->linesize);

		int y_size=pCodecCtx->width*pCodecCtx->height;  
		fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
		fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V

		printf("Flush Decoder: Succeed to decode 1 frame!\n");
	}

	sws_freeContext(img_convert_ctx);

    fclose(fp_yuv);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
