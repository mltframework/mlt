

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 19:14:07 2038
 */
/* Compiler settings for ..\..\..\Win\include\DeckLinkAPI.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __DeckLinkAPI_h__
#define __DeckLinkAPI_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __IDeckLinkTimecode_FWD_DEFINED__
#define __IDeckLinkTimecode_FWD_DEFINED__
typedef interface IDeckLinkTimecode IDeckLinkTimecode;

#endif 	/* __IDeckLinkTimecode_FWD_DEFINED__ */


#ifndef __IDeckLinkDisplayModeIterator_FWD_DEFINED__
#define __IDeckLinkDisplayModeIterator_FWD_DEFINED__
typedef interface IDeckLinkDisplayModeIterator IDeckLinkDisplayModeIterator;

#endif 	/* __IDeckLinkDisplayModeIterator_FWD_DEFINED__ */


#ifndef __IDeckLinkDisplayMode_FWD_DEFINED__
#define __IDeckLinkDisplayMode_FWD_DEFINED__
typedef interface IDeckLinkDisplayMode IDeckLinkDisplayMode;

#endif 	/* __IDeckLinkDisplayMode_FWD_DEFINED__ */


#ifndef __IDeckLink_FWD_DEFINED__
#define __IDeckLink_FWD_DEFINED__
typedef interface IDeckLink IDeckLink;

#endif 	/* __IDeckLink_FWD_DEFINED__ */


#ifndef __IDeckLinkConfiguration_FWD_DEFINED__
#define __IDeckLinkConfiguration_FWD_DEFINED__
typedef interface IDeckLinkConfiguration IDeckLinkConfiguration;

#endif 	/* __IDeckLinkConfiguration_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderConfiguration_FWD_DEFINED__
#define __IDeckLinkEncoderConfiguration_FWD_DEFINED__
typedef interface IDeckLinkEncoderConfiguration IDeckLinkEncoderConfiguration;

#endif 	/* __IDeckLinkEncoderConfiguration_FWD_DEFINED__ */


#ifndef __IDeckLinkDeckControlStatusCallback_FWD_DEFINED__
#define __IDeckLinkDeckControlStatusCallback_FWD_DEFINED__
typedef interface IDeckLinkDeckControlStatusCallback IDeckLinkDeckControlStatusCallback;

#endif 	/* __IDeckLinkDeckControlStatusCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkDeckControl_FWD_DEFINED__
#define __IDeckLinkDeckControl_FWD_DEFINED__
typedef interface IDeckLinkDeckControl IDeckLinkDeckControl;

#endif 	/* __IDeckLinkDeckControl_FWD_DEFINED__ */


#ifndef __IBMDStreamingDeviceNotificationCallback_FWD_DEFINED__
#define __IBMDStreamingDeviceNotificationCallback_FWD_DEFINED__
typedef interface IBMDStreamingDeviceNotificationCallback IBMDStreamingDeviceNotificationCallback;

#endif 	/* __IBMDStreamingDeviceNotificationCallback_FWD_DEFINED__ */


#ifndef __IBMDStreamingH264InputCallback_FWD_DEFINED__
#define __IBMDStreamingH264InputCallback_FWD_DEFINED__
typedef interface IBMDStreamingH264InputCallback IBMDStreamingH264InputCallback;

#endif 	/* __IBMDStreamingH264InputCallback_FWD_DEFINED__ */


#ifndef __IBMDStreamingDiscovery_FWD_DEFINED__
#define __IBMDStreamingDiscovery_FWD_DEFINED__
typedef interface IBMDStreamingDiscovery IBMDStreamingDiscovery;

#endif 	/* __IBMDStreamingDiscovery_FWD_DEFINED__ */


#ifndef __IBMDStreamingVideoEncodingMode_FWD_DEFINED__
#define __IBMDStreamingVideoEncodingMode_FWD_DEFINED__
typedef interface IBMDStreamingVideoEncodingMode IBMDStreamingVideoEncodingMode;

#endif 	/* __IBMDStreamingVideoEncodingMode_FWD_DEFINED__ */


#ifndef __IBMDStreamingMutableVideoEncodingMode_FWD_DEFINED__
#define __IBMDStreamingMutableVideoEncodingMode_FWD_DEFINED__
typedef interface IBMDStreamingMutableVideoEncodingMode IBMDStreamingMutableVideoEncodingMode;

#endif 	/* __IBMDStreamingMutableVideoEncodingMode_FWD_DEFINED__ */


#ifndef __IBMDStreamingVideoEncodingModePresetIterator_FWD_DEFINED__
#define __IBMDStreamingVideoEncodingModePresetIterator_FWD_DEFINED__
typedef interface IBMDStreamingVideoEncodingModePresetIterator IBMDStreamingVideoEncodingModePresetIterator;

#endif 	/* __IBMDStreamingVideoEncodingModePresetIterator_FWD_DEFINED__ */


#ifndef __IBMDStreamingDeviceInput_FWD_DEFINED__
#define __IBMDStreamingDeviceInput_FWD_DEFINED__
typedef interface IBMDStreamingDeviceInput IBMDStreamingDeviceInput;

#endif 	/* __IBMDStreamingDeviceInput_FWD_DEFINED__ */


#ifndef __IBMDStreamingH264NALPacket_FWD_DEFINED__
#define __IBMDStreamingH264NALPacket_FWD_DEFINED__
typedef interface IBMDStreamingH264NALPacket IBMDStreamingH264NALPacket;

#endif 	/* __IBMDStreamingH264NALPacket_FWD_DEFINED__ */


#ifndef __IBMDStreamingAudioPacket_FWD_DEFINED__
#define __IBMDStreamingAudioPacket_FWD_DEFINED__
typedef interface IBMDStreamingAudioPacket IBMDStreamingAudioPacket;

#endif 	/* __IBMDStreamingAudioPacket_FWD_DEFINED__ */


#ifndef __IBMDStreamingMPEG2TSPacket_FWD_DEFINED__
#define __IBMDStreamingMPEG2TSPacket_FWD_DEFINED__
typedef interface IBMDStreamingMPEG2TSPacket IBMDStreamingMPEG2TSPacket;

#endif 	/* __IBMDStreamingMPEG2TSPacket_FWD_DEFINED__ */


#ifndef __IBMDStreamingH264NALParser_FWD_DEFINED__
#define __IBMDStreamingH264NALParser_FWD_DEFINED__
typedef interface IBMDStreamingH264NALParser IBMDStreamingH264NALParser;

#endif 	/* __IBMDStreamingH264NALParser_FWD_DEFINED__ */


#ifndef __CBMDStreamingDiscovery_FWD_DEFINED__
#define __CBMDStreamingDiscovery_FWD_DEFINED__

#ifdef __cplusplus
typedef class CBMDStreamingDiscovery CBMDStreamingDiscovery;
#else
typedef struct CBMDStreamingDiscovery CBMDStreamingDiscovery;
#endif /* __cplusplus */

#endif 	/* __CBMDStreamingDiscovery_FWD_DEFINED__ */


#ifndef __CBMDStreamingH264NALParser_FWD_DEFINED__
#define __CBMDStreamingH264NALParser_FWD_DEFINED__

#ifdef __cplusplus
typedef class CBMDStreamingH264NALParser CBMDStreamingH264NALParser;
#else
typedef struct CBMDStreamingH264NALParser CBMDStreamingH264NALParser;
#endif /* __cplusplus */

#endif 	/* __CBMDStreamingH264NALParser_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoOutputCallback_FWD_DEFINED__
#define __IDeckLinkVideoOutputCallback_FWD_DEFINED__
typedef interface IDeckLinkVideoOutputCallback IDeckLinkVideoOutputCallback;

#endif 	/* __IDeckLinkVideoOutputCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkInputCallback_FWD_DEFINED__
#define __IDeckLinkInputCallback_FWD_DEFINED__
typedef interface IDeckLinkInputCallback IDeckLinkInputCallback;

#endif 	/* __IDeckLinkInputCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderInputCallback_FWD_DEFINED__
#define __IDeckLinkEncoderInputCallback_FWD_DEFINED__
typedef interface IDeckLinkEncoderInputCallback IDeckLinkEncoderInputCallback;

#endif 	/* __IDeckLinkEncoderInputCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoBufferAllocator_FWD_DEFINED__
#define __IDeckLinkVideoBufferAllocator_FWD_DEFINED__
typedef interface IDeckLinkVideoBufferAllocator IDeckLinkVideoBufferAllocator;

#endif 	/* __IDeckLinkVideoBufferAllocator_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoBufferAllocatorProvider_FWD_DEFINED__
#define __IDeckLinkVideoBufferAllocatorProvider_FWD_DEFINED__
typedef interface IDeckLinkVideoBufferAllocatorProvider IDeckLinkVideoBufferAllocatorProvider;

#endif 	/* __IDeckLinkVideoBufferAllocatorProvider_FWD_DEFINED__ */


#ifndef __IDeckLinkAudioOutputCallback_FWD_DEFINED__
#define __IDeckLinkAudioOutputCallback_FWD_DEFINED__
typedef interface IDeckLinkAudioOutputCallback IDeckLinkAudioOutputCallback;

#endif 	/* __IDeckLinkAudioOutputCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkIterator_FWD_DEFINED__
#define __IDeckLinkIterator_FWD_DEFINED__
typedef interface IDeckLinkIterator IDeckLinkIterator;

#endif 	/* __IDeckLinkIterator_FWD_DEFINED__ */


#ifndef __IDeckLinkAPIInformation_FWD_DEFINED__
#define __IDeckLinkAPIInformation_FWD_DEFINED__
typedef interface IDeckLinkAPIInformation IDeckLinkAPIInformation;

#endif 	/* __IDeckLinkAPIInformation_FWD_DEFINED__ */


#ifndef __IDeckLinkIPFlowAttributes_FWD_DEFINED__
#define __IDeckLinkIPFlowAttributes_FWD_DEFINED__
typedef interface IDeckLinkIPFlowAttributes IDeckLinkIPFlowAttributes;

#endif 	/* __IDeckLinkIPFlowAttributes_FWD_DEFINED__ */


#ifndef __IDeckLinkIPFlowStatus_FWD_DEFINED__
#define __IDeckLinkIPFlowStatus_FWD_DEFINED__
typedef interface IDeckLinkIPFlowStatus IDeckLinkIPFlowStatus;

#endif 	/* __IDeckLinkIPFlowStatus_FWD_DEFINED__ */


#ifndef __IDeckLinkIPFlowSetting_FWD_DEFINED__
#define __IDeckLinkIPFlowSetting_FWD_DEFINED__
typedef interface IDeckLinkIPFlowSetting IDeckLinkIPFlowSetting;

#endif 	/* __IDeckLinkIPFlowSetting_FWD_DEFINED__ */


#ifndef __IDeckLinkIPFlow_FWD_DEFINED__
#define __IDeckLinkIPFlow_FWD_DEFINED__
typedef interface IDeckLinkIPFlow IDeckLinkIPFlow;

#endif 	/* __IDeckLinkIPFlow_FWD_DEFINED__ */


#ifndef __IDeckLinkIPFlowIterator_FWD_DEFINED__
#define __IDeckLinkIPFlowIterator_FWD_DEFINED__
typedef interface IDeckLinkIPFlowIterator IDeckLinkIPFlowIterator;

#endif 	/* __IDeckLinkIPFlowIterator_FWD_DEFINED__ */


#ifndef __IDeckLinkOutput_FWD_DEFINED__
#define __IDeckLinkOutput_FWD_DEFINED__
typedef interface IDeckLinkOutput IDeckLinkOutput;

#endif 	/* __IDeckLinkOutput_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_FWD_DEFINED__
#define __IDeckLinkInput_FWD_DEFINED__
typedef interface IDeckLinkInput IDeckLinkInput;

#endif 	/* __IDeckLinkInput_FWD_DEFINED__ */


#ifndef __IDeckLinkIPExtensions_FWD_DEFINED__
#define __IDeckLinkIPExtensions_FWD_DEFINED__
typedef interface IDeckLinkIPExtensions IDeckLinkIPExtensions;

#endif 	/* __IDeckLinkIPExtensions_FWD_DEFINED__ */


#ifndef __IDeckLinkHDMIInputEDID_FWD_DEFINED__
#define __IDeckLinkHDMIInputEDID_FWD_DEFINED__
typedef interface IDeckLinkHDMIInputEDID IDeckLinkHDMIInputEDID;

#endif 	/* __IDeckLinkHDMIInputEDID_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderInput_FWD_DEFINED__
#define __IDeckLinkEncoderInput_FWD_DEFINED__
typedef interface IDeckLinkEncoderInput IDeckLinkEncoderInput;

#endif 	/* __IDeckLinkEncoderInput_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoBuffer_FWD_DEFINED__
#define __IDeckLinkVideoBuffer_FWD_DEFINED__
typedef interface IDeckLinkVideoBuffer IDeckLinkVideoBuffer;

#endif 	/* __IDeckLinkVideoBuffer_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_FWD_DEFINED__
#define __IDeckLinkVideoFrame_FWD_DEFINED__
typedef interface IDeckLinkVideoFrame IDeckLinkVideoFrame;

#endif 	/* __IDeckLinkVideoFrame_FWD_DEFINED__ */


#ifndef __IDeckLinkMutableVideoFrame_FWD_DEFINED__
#define __IDeckLinkMutableVideoFrame_FWD_DEFINED__
typedef interface IDeckLinkMutableVideoFrame IDeckLinkMutableVideoFrame;

#endif 	/* __IDeckLinkMutableVideoFrame_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrame3DExtensions_FWD_DEFINED__
#define __IDeckLinkVideoFrame3DExtensions_FWD_DEFINED__
typedef interface IDeckLinkVideoFrame3DExtensions IDeckLinkVideoFrame3DExtensions;

#endif 	/* __IDeckLinkVideoFrame3DExtensions_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrameMetadataExtensions_FWD_DEFINED__
#define __IDeckLinkVideoFrameMetadataExtensions_FWD_DEFINED__
typedef interface IDeckLinkVideoFrameMetadataExtensions IDeckLinkVideoFrameMetadataExtensions;

#endif 	/* __IDeckLinkVideoFrameMetadataExtensions_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrameMutableMetadataExtensions_FWD_DEFINED__
#define __IDeckLinkVideoFrameMutableMetadataExtensions_FWD_DEFINED__
typedef interface IDeckLinkVideoFrameMutableMetadataExtensions IDeckLinkVideoFrameMutableMetadataExtensions;

#endif 	/* __IDeckLinkVideoFrameMutableMetadataExtensions_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_FWD_DEFINED__
#define __IDeckLinkVideoInputFrame_FWD_DEFINED__
typedef interface IDeckLinkVideoInputFrame IDeckLinkVideoInputFrame;

#endif 	/* __IDeckLinkVideoInputFrame_FWD_DEFINED__ */


#ifndef __IDeckLinkAncillaryPacket_FWD_DEFINED__
#define __IDeckLinkAncillaryPacket_FWD_DEFINED__
typedef interface IDeckLinkAncillaryPacket IDeckLinkAncillaryPacket;

#endif 	/* __IDeckLinkAncillaryPacket_FWD_DEFINED__ */


#ifndef __IDeckLinkAncillaryPacketIterator_FWD_DEFINED__
#define __IDeckLinkAncillaryPacketIterator_FWD_DEFINED__
typedef interface IDeckLinkAncillaryPacketIterator IDeckLinkAncillaryPacketIterator;

#endif 	/* __IDeckLinkAncillaryPacketIterator_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrameAncillaryPackets_FWD_DEFINED__
#define __IDeckLinkVideoFrameAncillaryPackets_FWD_DEFINED__
typedef interface IDeckLinkVideoFrameAncillaryPackets IDeckLinkVideoFrameAncillaryPackets;

#endif 	/* __IDeckLinkVideoFrameAncillaryPackets_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrameAncillary_FWD_DEFINED__
#define __IDeckLinkVideoFrameAncillary_FWD_DEFINED__
typedef interface IDeckLinkVideoFrameAncillary IDeckLinkVideoFrameAncillary;

#endif 	/* __IDeckLinkVideoFrameAncillary_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderPacket_FWD_DEFINED__
#define __IDeckLinkEncoderPacket_FWD_DEFINED__
typedef interface IDeckLinkEncoderPacket IDeckLinkEncoderPacket;

#endif 	/* __IDeckLinkEncoderPacket_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderVideoPacket_FWD_DEFINED__
#define __IDeckLinkEncoderVideoPacket_FWD_DEFINED__
typedef interface IDeckLinkEncoderVideoPacket IDeckLinkEncoderVideoPacket;

#endif 	/* __IDeckLinkEncoderVideoPacket_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderAudioPacket_FWD_DEFINED__
#define __IDeckLinkEncoderAudioPacket_FWD_DEFINED__
typedef interface IDeckLinkEncoderAudioPacket IDeckLinkEncoderAudioPacket;

#endif 	/* __IDeckLinkEncoderAudioPacket_FWD_DEFINED__ */


#ifndef __IDeckLinkH265NALPacket_FWD_DEFINED__
#define __IDeckLinkH265NALPacket_FWD_DEFINED__
typedef interface IDeckLinkH265NALPacket IDeckLinkH265NALPacket;

#endif 	/* __IDeckLinkH265NALPacket_FWD_DEFINED__ */


#ifndef __IDeckLinkAudioInputPacket_FWD_DEFINED__
#define __IDeckLinkAudioInputPacket_FWD_DEFINED__
typedef interface IDeckLinkAudioInputPacket IDeckLinkAudioInputPacket;

#endif 	/* __IDeckLinkAudioInputPacket_FWD_DEFINED__ */


#ifndef __IDeckLinkScreenPreviewCallback_FWD_DEFINED__
#define __IDeckLinkScreenPreviewCallback_FWD_DEFINED__
typedef interface IDeckLinkScreenPreviewCallback IDeckLinkScreenPreviewCallback;

#endif 	/* __IDeckLinkScreenPreviewCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkGLScreenPreviewHelper_FWD_DEFINED__
#define __IDeckLinkGLScreenPreviewHelper_FWD_DEFINED__
typedef interface IDeckLinkGLScreenPreviewHelper IDeckLinkGLScreenPreviewHelper;

#endif 	/* __IDeckLinkGLScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __IDeckLinkDX9ScreenPreviewHelper_FWD_DEFINED__
#define __IDeckLinkDX9ScreenPreviewHelper_FWD_DEFINED__
typedef interface IDeckLinkDX9ScreenPreviewHelper IDeckLinkDX9ScreenPreviewHelper;

#endif 	/* __IDeckLinkDX9ScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __IDeckLinkWPFDX9ScreenPreviewHelper_FWD_DEFINED__
#define __IDeckLinkWPFDX9ScreenPreviewHelper_FWD_DEFINED__
typedef interface IDeckLinkWPFDX9ScreenPreviewHelper IDeckLinkWPFDX9ScreenPreviewHelper;

#endif 	/* __IDeckLinkWPFDX9ScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __IDeckLinkNotificationCallback_FWD_DEFINED__
#define __IDeckLinkNotificationCallback_FWD_DEFINED__
typedef interface IDeckLinkNotificationCallback IDeckLinkNotificationCallback;

#endif 	/* __IDeckLinkNotificationCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkNotification_FWD_DEFINED__
#define __IDeckLinkNotification_FWD_DEFINED__
typedef interface IDeckLinkNotification IDeckLinkNotification;

#endif 	/* __IDeckLinkNotification_FWD_DEFINED__ */


#ifndef __IDeckLinkProfileAttributes_FWD_DEFINED__
#define __IDeckLinkProfileAttributes_FWD_DEFINED__
typedef interface IDeckLinkProfileAttributes IDeckLinkProfileAttributes;

#endif 	/* __IDeckLinkProfileAttributes_FWD_DEFINED__ */


#ifndef __IDeckLinkProfileIterator_FWD_DEFINED__
#define __IDeckLinkProfileIterator_FWD_DEFINED__
typedef interface IDeckLinkProfileIterator IDeckLinkProfileIterator;

#endif 	/* __IDeckLinkProfileIterator_FWD_DEFINED__ */


#ifndef __IDeckLinkProfile_FWD_DEFINED__
#define __IDeckLinkProfile_FWD_DEFINED__
typedef interface IDeckLinkProfile IDeckLinkProfile;

#endif 	/* __IDeckLinkProfile_FWD_DEFINED__ */


#ifndef __IDeckLinkProfileCallback_FWD_DEFINED__
#define __IDeckLinkProfileCallback_FWD_DEFINED__
typedef interface IDeckLinkProfileCallback IDeckLinkProfileCallback;

#endif 	/* __IDeckLinkProfileCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkProfileManager_FWD_DEFINED__
#define __IDeckLinkProfileManager_FWD_DEFINED__
typedef interface IDeckLinkProfileManager IDeckLinkProfileManager;

#endif 	/* __IDeckLinkProfileManager_FWD_DEFINED__ */


#ifndef __IDeckLinkStatus_FWD_DEFINED__
#define __IDeckLinkStatus_FWD_DEFINED__
typedef interface IDeckLinkStatus IDeckLinkStatus;

#endif 	/* __IDeckLinkStatus_FWD_DEFINED__ */


#ifndef __IDeckLinkKeyer_FWD_DEFINED__
#define __IDeckLinkKeyer_FWD_DEFINED__
typedef interface IDeckLinkKeyer IDeckLinkKeyer;

#endif 	/* __IDeckLinkKeyer_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoConversion_FWD_DEFINED__
#define __IDeckLinkVideoConversion_FWD_DEFINED__
typedef interface IDeckLinkVideoConversion IDeckLinkVideoConversion;

#endif 	/* __IDeckLinkVideoConversion_FWD_DEFINED__ */


#ifndef __IDeckLinkDeviceNotificationCallback_FWD_DEFINED__
#define __IDeckLinkDeviceNotificationCallback_FWD_DEFINED__
typedef interface IDeckLinkDeviceNotificationCallback IDeckLinkDeviceNotificationCallback;

#endif 	/* __IDeckLinkDeviceNotificationCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkDiscovery_FWD_DEFINED__
#define __IDeckLinkDiscovery_FWD_DEFINED__
typedef interface IDeckLinkDiscovery IDeckLinkDiscovery;

#endif 	/* __IDeckLinkDiscovery_FWD_DEFINED__ */


#ifndef __CDeckLinkIterator_FWD_DEFINED__
#define __CDeckLinkIterator_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkIterator CDeckLinkIterator;
#else
typedef struct CDeckLinkIterator CDeckLinkIterator;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkIterator_FWD_DEFINED__ */


#ifndef __CDeckLinkAPIInformation_FWD_DEFINED__
#define __CDeckLinkAPIInformation_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkAPIInformation CDeckLinkAPIInformation;
#else
typedef struct CDeckLinkAPIInformation CDeckLinkAPIInformation;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkAPIInformation_FWD_DEFINED__ */


#ifndef __CDeckLinkGLScreenPreviewHelper_FWD_DEFINED__
#define __CDeckLinkGLScreenPreviewHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkGLScreenPreviewHelper CDeckLinkGLScreenPreviewHelper;
#else
typedef struct CDeckLinkGLScreenPreviewHelper CDeckLinkGLScreenPreviewHelper;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkGLScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __CDeckLinkGL3ScreenPreviewHelper_FWD_DEFINED__
#define __CDeckLinkGL3ScreenPreviewHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkGL3ScreenPreviewHelper CDeckLinkGL3ScreenPreviewHelper;
#else
typedef struct CDeckLinkGL3ScreenPreviewHelper CDeckLinkGL3ScreenPreviewHelper;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkGL3ScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __CDeckLinkDX9ScreenPreviewHelper_FWD_DEFINED__
#define __CDeckLinkDX9ScreenPreviewHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkDX9ScreenPreviewHelper CDeckLinkDX9ScreenPreviewHelper;
#else
typedef struct CDeckLinkDX9ScreenPreviewHelper CDeckLinkDX9ScreenPreviewHelper;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkDX9ScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __CDeckLinkWPFDX9ScreenPreviewHelper_FWD_DEFINED__
#define __CDeckLinkWPFDX9ScreenPreviewHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkWPFDX9ScreenPreviewHelper CDeckLinkWPFDX9ScreenPreviewHelper;
#else
typedef struct CDeckLinkWPFDX9ScreenPreviewHelper CDeckLinkWPFDX9ScreenPreviewHelper;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkWPFDX9ScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __CDeckLinkVideoConversion_FWD_DEFINED__
#define __CDeckLinkVideoConversion_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkVideoConversion CDeckLinkVideoConversion;
#else
typedef struct CDeckLinkVideoConversion CDeckLinkVideoConversion;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkVideoConversion_FWD_DEFINED__ */


#ifndef __CDeckLinkDiscovery_FWD_DEFINED__
#define __CDeckLinkDiscovery_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkDiscovery CDeckLinkDiscovery;
#else
typedef struct CDeckLinkDiscovery CDeckLinkDiscovery;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkDiscovery_FWD_DEFINED__ */


#ifndef __CDeckLinkVideoFrameAncillaryPackets_FWD_DEFINED__
#define __CDeckLinkVideoFrameAncillaryPackets_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkVideoFrameAncillaryPackets CDeckLinkVideoFrameAncillaryPackets;
#else
typedef struct CDeckLinkVideoFrameAncillaryPackets CDeckLinkVideoFrameAncillaryPackets;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkVideoFrameAncillaryPackets_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoOutputCallback_v14_2_1_FWD_DEFINED__
#define __IDeckLinkVideoOutputCallback_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkVideoOutputCallback_v14_2_1 IDeckLinkVideoOutputCallback_v14_2_1;

#endif 	/* __IDeckLinkVideoOutputCallback_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkInputCallback_v14_2_1_FWD_DEFINED__
#define __IDeckLinkInputCallback_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkInputCallback_v14_2_1 IDeckLinkInputCallback_v14_2_1;

#endif 	/* __IDeckLinkInputCallback_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkMemoryAllocator_v14_2_1_FWD_DEFINED__
#define __IDeckLinkMemoryAllocator_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkMemoryAllocator_v14_2_1 IDeckLinkMemoryAllocator_v14_2_1;

#endif 	/* __IDeckLinkMemoryAllocator_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkOutput_v14_2_1_FWD_DEFINED__
#define __IDeckLinkOutput_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkOutput_v14_2_1 IDeckLinkOutput_v14_2_1;

#endif 	/* __IDeckLinkOutput_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_v14_2_1_FWD_DEFINED__
#define __IDeckLinkInput_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkInput_v14_2_1 IDeckLinkInput_v14_2_1;

#endif 	/* __IDeckLinkInput_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderInput_v14_2_1_FWD_DEFINED__
#define __IDeckLinkEncoderInput_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkEncoderInput_v14_2_1 IDeckLinkEncoderInput_v14_2_1;

#endif 	/* __IDeckLinkEncoderInput_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_v14_2_1_FWD_DEFINED__
#define __IDeckLinkVideoFrame_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkVideoFrame_v14_2_1 IDeckLinkVideoFrame_v14_2_1;

#endif 	/* __IDeckLinkVideoFrame_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkMutableVideoFrame_v14_2_1_FWD_DEFINED__
#define __IDeckLinkMutableVideoFrame_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkMutableVideoFrame_v14_2_1 IDeckLinkMutableVideoFrame_v14_2_1;

#endif 	/* __IDeckLinkMutableVideoFrame_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrame3DExtensions_v14_2_1_FWD_DEFINED__
#define __IDeckLinkVideoFrame3DExtensions_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkVideoFrame3DExtensions_v14_2_1 IDeckLinkVideoFrame3DExtensions_v14_2_1;

#endif 	/* __IDeckLinkVideoFrame3DExtensions_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_v14_2_1_FWD_DEFINED__
#define __IDeckLinkVideoInputFrame_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkVideoInputFrame_v14_2_1 IDeckLinkVideoInputFrame_v14_2_1;

#endif 	/* __IDeckLinkVideoInputFrame_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkScreenPreviewCallback_v14_2_1_FWD_DEFINED__
#define __IDeckLinkScreenPreviewCallback_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkScreenPreviewCallback_v14_2_1 IDeckLinkScreenPreviewCallback_v14_2_1;

#endif 	/* __IDeckLinkScreenPreviewCallback_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkGLScreenPreviewHelper_v14_2_1_FWD_DEFINED__
#define __IDeckLinkGLScreenPreviewHelper_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkGLScreenPreviewHelper_v14_2_1 IDeckLinkGLScreenPreviewHelper_v14_2_1;

#endif 	/* __IDeckLinkGLScreenPreviewHelper_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__
#define __IDeckLinkDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkDX9ScreenPreviewHelper_v14_2_1 IDeckLinkDX9ScreenPreviewHelper_v14_2_1;

#endif 	/* __IDeckLinkDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__
#define __IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1;

#endif 	/* __IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoConversion_v14_2_1_FWD_DEFINED__
#define __IDeckLinkVideoConversion_v14_2_1_FWD_DEFINED__
typedef interface IDeckLinkVideoConversion_v14_2_1 IDeckLinkVideoConversion_v14_2_1;

#endif 	/* __IDeckLinkVideoConversion_v14_2_1_FWD_DEFINED__ */


#ifndef __CDeckLinkGLScreenPreviewHelper_v14_2_1_FWD_DEFINED__
#define __CDeckLinkGLScreenPreviewHelper_v14_2_1_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkGLScreenPreviewHelper_v14_2_1 CDeckLinkGLScreenPreviewHelper_v14_2_1;
#else
typedef struct CDeckLinkGLScreenPreviewHelper_v14_2_1 CDeckLinkGLScreenPreviewHelper_v14_2_1;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkGLScreenPreviewHelper_v14_2_1_FWD_DEFINED__ */


#ifndef __CDeckLinkGL3ScreenPreviewHelper_v14_2_1_FWD_DEFINED__
#define __CDeckLinkGL3ScreenPreviewHelper_v14_2_1_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkGL3ScreenPreviewHelper_v14_2_1 CDeckLinkGL3ScreenPreviewHelper_v14_2_1;
#else
typedef struct CDeckLinkGL3ScreenPreviewHelper_v14_2_1 CDeckLinkGL3ScreenPreviewHelper_v14_2_1;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkGL3ScreenPreviewHelper_v14_2_1_FWD_DEFINED__ */


#ifndef __CDeckLinkDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__
#define __CDeckLinkDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkDX9ScreenPreviewHelper_v14_2_1 CDeckLinkDX9ScreenPreviewHelper_v14_2_1;
#else
typedef struct CDeckLinkDX9ScreenPreviewHelper_v14_2_1 CDeckLinkDX9ScreenPreviewHelper_v14_2_1;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__ */


#ifndef __CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__
#define __CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1;
#else
typedef struct CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_FWD_DEFINED__ */


#ifndef __CDeckLinkVideoConversion_v14_2_1_FWD_DEFINED__
#define __CDeckLinkVideoConversion_v14_2_1_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkVideoConversion_v14_2_1 CDeckLinkVideoConversion_v14_2_1;
#else
typedef struct CDeckLinkVideoConversion_v14_2_1 CDeckLinkVideoConversion_v14_2_1;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkVideoConversion_v14_2_1_FWD_DEFINED__ */


#ifndef __IDeckLinkInputCallback_v11_5_1_FWD_DEFINED__
#define __IDeckLinkInputCallback_v11_5_1_FWD_DEFINED__
typedef interface IDeckLinkInputCallback_v11_5_1 IDeckLinkInputCallback_v11_5_1;

#endif 	/* __IDeckLinkInputCallback_v11_5_1_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_v11_5_1_FWD_DEFINED__
#define __IDeckLinkInput_v11_5_1_FWD_DEFINED__
typedef interface IDeckLinkInput_v11_5_1 IDeckLinkInput_v11_5_1;

#endif 	/* __IDeckLinkInput_v11_5_1_FWD_DEFINED__ */


#ifndef __IDeckLinkConfiguration_v10_11_FWD_DEFINED__
#define __IDeckLinkConfiguration_v10_11_FWD_DEFINED__
typedef interface IDeckLinkConfiguration_v10_11 IDeckLinkConfiguration_v10_11;

#endif 	/* __IDeckLinkConfiguration_v10_11_FWD_DEFINED__ */


#ifndef __IDeckLinkAttributes_v10_11_FWD_DEFINED__
#define __IDeckLinkAttributes_v10_11_FWD_DEFINED__
typedef interface IDeckLinkAttributes_v10_11 IDeckLinkAttributes_v10_11;

#endif 	/* __IDeckLinkAttributes_v10_11_FWD_DEFINED__ */


#ifndef __IDeckLinkNotification_v10_11_FWD_DEFINED__
#define __IDeckLinkNotification_v10_11_FWD_DEFINED__
typedef interface IDeckLinkNotification_v10_11 IDeckLinkNotification_v10_11;

#endif 	/* __IDeckLinkNotification_v10_11_FWD_DEFINED__ */


#ifndef __IDeckLinkOutput_v10_11_FWD_DEFINED__
#define __IDeckLinkOutput_v10_11_FWD_DEFINED__
typedef interface IDeckLinkOutput_v10_11 IDeckLinkOutput_v10_11;

#endif 	/* __IDeckLinkOutput_v10_11_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_v10_11_FWD_DEFINED__
#define __IDeckLinkInput_v10_11_FWD_DEFINED__
typedef interface IDeckLinkInput_v10_11 IDeckLinkInput_v10_11;

#endif 	/* __IDeckLinkInput_v10_11_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderInput_v10_11_FWD_DEFINED__
#define __IDeckLinkEncoderInput_v10_11_FWD_DEFINED__
typedef interface IDeckLinkEncoderInput_v10_11 IDeckLinkEncoderInput_v10_11;

#endif 	/* __IDeckLinkEncoderInput_v10_11_FWD_DEFINED__ */


#ifndef __CDeckLinkIterator_v10_11_FWD_DEFINED__
#define __CDeckLinkIterator_v10_11_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkIterator_v10_11 CDeckLinkIterator_v10_11;
#else
typedef struct CDeckLinkIterator_v10_11 CDeckLinkIterator_v10_11;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkIterator_v10_11_FWD_DEFINED__ */


#ifndef __CDeckLinkDiscovery_v10_11_FWD_DEFINED__
#define __CDeckLinkDiscovery_v10_11_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkDiscovery_v10_11 CDeckLinkDiscovery_v10_11;
#else
typedef struct CDeckLinkDiscovery_v10_11 CDeckLinkDiscovery_v10_11;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkDiscovery_v10_11_FWD_DEFINED__ */


#ifndef __IDeckLinkConfiguration_v10_9_FWD_DEFINED__
#define __IDeckLinkConfiguration_v10_9_FWD_DEFINED__
typedef interface IDeckLinkConfiguration_v10_9 IDeckLinkConfiguration_v10_9;

#endif 	/* __IDeckLinkConfiguration_v10_9_FWD_DEFINED__ */


#ifndef __CBMDStreamingDiscovery_v10_8_FWD_DEFINED__
#define __CBMDStreamingDiscovery_v10_8_FWD_DEFINED__

#ifdef __cplusplus
typedef class CBMDStreamingDiscovery_v10_8 CBMDStreamingDiscovery_v10_8;
#else
typedef struct CBMDStreamingDiscovery_v10_8 CBMDStreamingDiscovery_v10_8;
#endif /* __cplusplus */

#endif 	/* __CBMDStreamingDiscovery_v10_8_FWD_DEFINED__ */


#ifndef __IDeckLinkConfiguration_v10_4_FWD_DEFINED__
#define __IDeckLinkConfiguration_v10_4_FWD_DEFINED__
typedef interface IDeckLinkConfiguration_v10_4 IDeckLinkConfiguration_v10_4;

#endif 	/* __IDeckLinkConfiguration_v10_4_FWD_DEFINED__ */


#ifndef __IDeckLinkConfiguration_v10_2_FWD_DEFINED__
#define __IDeckLinkConfiguration_v10_2_FWD_DEFINED__
typedef interface IDeckLinkConfiguration_v10_2 IDeckLinkConfiguration_v10_2;

#endif 	/* __IDeckLinkConfiguration_v10_2_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrameMetadataExtensions_v11_5_FWD_DEFINED__
#define __IDeckLinkVideoFrameMetadataExtensions_v11_5_FWD_DEFINED__
typedef interface IDeckLinkVideoFrameMetadataExtensions_v11_5 IDeckLinkVideoFrameMetadataExtensions_v11_5;

#endif 	/* __IDeckLinkVideoFrameMetadataExtensions_v11_5_FWD_DEFINED__ */


#ifndef __IDeckLinkOutput_v11_4_FWD_DEFINED__
#define __IDeckLinkOutput_v11_4_FWD_DEFINED__
typedef interface IDeckLinkOutput_v11_4 IDeckLinkOutput_v11_4;

#endif 	/* __IDeckLinkOutput_v11_4_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_v11_4_FWD_DEFINED__
#define __IDeckLinkInput_v11_4_FWD_DEFINED__
typedef interface IDeckLinkInput_v11_4 IDeckLinkInput_v11_4;

#endif 	/* __IDeckLinkInput_v11_4_FWD_DEFINED__ */


#ifndef __CDeckLinkIterator_v10_8_FWD_DEFINED__
#define __CDeckLinkIterator_v10_8_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkIterator_v10_8 CDeckLinkIterator_v10_8;
#else
typedef struct CDeckLinkIterator_v10_8 CDeckLinkIterator_v10_8;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkIterator_v10_8_FWD_DEFINED__ */


#ifndef __CDeckLinkDiscovery_v10_8_FWD_DEFINED__
#define __CDeckLinkDiscovery_v10_8_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkDiscovery_v10_8 CDeckLinkDiscovery_v10_8;
#else
typedef struct CDeckLinkDiscovery_v10_8 CDeckLinkDiscovery_v10_8;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkDiscovery_v10_8_FWD_DEFINED__ */


#ifndef __IDeckLinkEncoderConfiguration_v10_5_FWD_DEFINED__
#define __IDeckLinkEncoderConfiguration_v10_5_FWD_DEFINED__
typedef interface IDeckLinkEncoderConfiguration_v10_5 IDeckLinkEncoderConfiguration_v10_5;

#endif 	/* __IDeckLinkEncoderConfiguration_v10_5_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __DeckLinkAPI_LIBRARY_DEFINED__
#define __DeckLinkAPI_LIBRARY_DEFINED__

/* library DeckLinkAPI */
/* [helpstring][version][uuid] */ 

typedef LONGLONG BMDTimeValue;

typedef LONGLONG BMDTimeScale;

typedef unsigned int BMDTimecodeBCD;

typedef unsigned int BMDTimecodeUserBits;

typedef LONGLONG BMDIPFlowID;

typedef unsigned int BMDTimecodeFlags;
#if 0
typedef enum _BMDTimecodeFlags BMDTimecodeFlags;

#endif
/* [v1_enum] */ 
enum _BMDTimecodeFlags
    {
        bmdTimecodeFlagDefault	= 0,
        bmdTimecodeIsDropFrame	= ( 1 << 0 ) ,
        bmdTimecodeFieldMark	= ( 1 << 1 ) ,
        bmdTimecodeColorFrame	= ( 1 << 2 ) ,
        bmdTimecodeEmbedRecordingTrigger	= ( 1 << 3 ) ,
        bmdTimecodeRecordingTriggered	= ( 1 << 4 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDVideoConnection
    {
        bmdVideoConnectionUnspecified	= 0,
        bmdVideoConnectionSDI	= ( 1 << 0 ) ,
        bmdVideoConnectionHDMI	= ( 1 << 1 ) ,
        bmdVideoConnectionOpticalSDI	= ( 1 << 2 ) ,
        bmdVideoConnectionComponent	= ( 1 << 3 ) ,
        bmdVideoConnectionComposite	= ( 1 << 4 ) ,
        bmdVideoConnectionSVideo	= ( 1 << 5 ) ,
        bmdVideoConnectionEthernet	= ( 1 << 6 ) ,
        bmdVideoConnectionOpticalEthernet	= ( 1 << 7 ) 
    } 	BMDVideoConnection;

typedef /* [v1_enum] */ 
enum _BMDAudioConnection
    {
        bmdAudioConnectionEmbedded	= ( 1 << 0 ) ,
        bmdAudioConnectionAESEBU	= ( 1 << 1 ) ,
        bmdAudioConnectionAnalog	= ( 1 << 2 ) ,
        bmdAudioConnectionAnalogXLR	= ( 1 << 3 ) ,
        bmdAudioConnectionAnalogRCA	= ( 1 << 4 ) ,
        bmdAudioConnectionMicrophone	= ( 1 << 5 ) ,
        bmdAudioConnectionHeadphones	= ( 1 << 6 ) 
    } 	BMDAudioConnection;

typedef /* [v1_enum] */ 
enum _BMDDeckControlConnection
    {
        bmdDeckControlConnectionRS422Remote1	= ( 1 << 0 ) ,
        bmdDeckControlConnectionRS422Remote2	= ( 1 << 1 ) 
    } 	BMDDeckControlConnection;


typedef unsigned int BMDDisplayModeFlags;
#if 0
typedef enum _BMDDisplayModeFlags BMDDisplayModeFlags;

#endif
typedef /* [v1_enum] */ 
enum _BMDDisplayMode
    {
        bmdModeNTSC	= 0x6e747363,
        bmdModeNTSC2398	= 0x6e743233,
        bmdModePAL	= 0x70616c20,
        bmdModeNTSCp	= 0x6e747370,
        bmdModePALp	= 0x70616c70,
        bmdModeHD1080p2398	= 0x32337073,
        bmdModeHD1080p24	= 0x32347073,
        bmdModeHD1080p25	= 0x48703235,
        bmdModeHD1080p2997	= 0x48703239,
        bmdModeHD1080p30	= 0x48703330,
        bmdModeHD1080p4795	= 0x48703437,
        bmdModeHD1080p48	= 0x48703438,
        bmdModeHD1080p50	= 0x48703530,
        bmdModeHD1080p5994	= 0x48703539,
        bmdModeHD1080p6000	= 0x48703630,
        bmdModeHD1080p9590	= 0x48703935,
        bmdModeHD1080p96	= 0x48703936,
        bmdModeHD1080p100	= 0x48703130,
        bmdModeHD1080p11988	= 0x48703131,
        bmdModeHD1080p120	= 0x48703132,
        bmdModeHD1080i50	= 0x48693530,
        bmdModeHD1080i5994	= 0x48693539,
        bmdModeHD1080i6000	= 0x48693630,
        bmdModeHD720p50	= 0x68703530,
        bmdModeHD720p5994	= 0x68703539,
        bmdModeHD720p60	= 0x68703630,
        bmdMode2k2398	= 0x326b3233,
        bmdMode2k24	= 0x326b3234,
        bmdMode2k25	= 0x326b3235,
        bmdMode2kDCI2398	= 0x32643233,
        bmdMode2kDCI24	= 0x32643234,
        bmdMode2kDCI25	= 0x32643235,
        bmdMode2kDCI2997	= 0x32643239,
        bmdMode2kDCI30	= 0x32643330,
        bmdMode2kDCI4795	= 0x32643437,
        bmdMode2kDCI48	= 0x32643438,
        bmdMode2kDCI50	= 0x32643530,
        bmdMode2kDCI5994	= 0x32643539,
        bmdMode2kDCI60	= 0x32643630,
        bmdMode2kDCI9590	= 0x32643935,
        bmdMode2kDCI96	= 0x32643936,
        bmdMode2kDCI100	= 0x32643130,
        bmdMode2kDCI11988	= 0x32643131,
        bmdMode2kDCI120	= 0x32643132,
        bmdMode4K2160p2398	= 0x346b3233,
        bmdMode4K2160p24	= 0x346b3234,
        bmdMode4K2160p25	= 0x346b3235,
        bmdMode4K2160p2997	= 0x346b3239,
        bmdMode4K2160p30	= 0x346b3330,
        bmdMode4K2160p4795	= 0x346b3437,
        bmdMode4K2160p48	= 0x346b3438,
        bmdMode4K2160p50	= 0x346b3530,
        bmdMode4K2160p5994	= 0x346b3539,
        bmdMode4K2160p60	= 0x346b3630,
        bmdMode4K2160p9590	= 0x346b3935,
        bmdMode4K2160p96	= 0x346b3936,
        bmdMode4K2160p100	= 0x346b3130,
        bmdMode4K2160p11988	= 0x346b3131,
        bmdMode4K2160p120	= 0x346b3132,
        bmdMode4kDCI2398	= 0x34643233,
        bmdMode4kDCI24	= 0x34643234,
        bmdMode4kDCI25	= 0x34643235,
        bmdMode4kDCI2997	= 0x34643239,
        bmdMode4kDCI30	= 0x34643330,
        bmdMode4kDCI4795	= 0x34643437,
        bmdMode4kDCI48	= 0x34643438,
        bmdMode4kDCI50	= 0x34643530,
        bmdMode4kDCI5994	= 0x34643539,
        bmdMode4kDCI60	= 0x34643630,
        bmdMode4kDCI9590	= 0x34643935,
        bmdMode4kDCI96	= 0x34643936,
        bmdMode4kDCI100	= 0x34643130,
        bmdMode4kDCI11988	= 0x34643131,
        bmdMode4kDCI120	= 0x34643132,
        bmdMode8K4320p2398	= 0x386b3233,
        bmdMode8K4320p24	= 0x386b3234,
        bmdMode8K4320p25	= 0x386b3235,
        bmdMode8K4320p2997	= 0x386b3239,
        bmdMode8K4320p30	= 0x386b3330,
        bmdMode8K4320p4795	= 0x386b3437,
        bmdMode8K4320p48	= 0x386b3438,
        bmdMode8K4320p50	= 0x386b3530,
        bmdMode8K4320p5994	= 0x386b3539,
        bmdMode8K4320p60	= 0x386b3630,
        bmdMode8kDCI2398	= 0x38643233,
        bmdMode8kDCI24	= 0x38643234,
        bmdMode8kDCI25	= 0x38643235,
        bmdMode8kDCI2997	= 0x38643239,
        bmdMode8kDCI30	= 0x38643330,
        bmdMode8kDCI4795	= 0x38643437,
        bmdMode8kDCI48	= 0x38643438,
        bmdMode8kDCI50	= 0x38643530,
        bmdMode8kDCI5994	= 0x38643539,
        bmdMode8kDCI60	= 0x38643630,
        bmdMode640x480p60	= 0x76676136,
        bmdMode800x600p60	= 0x73766736,
        bmdMode1440x900p50	= 0x77786735,
        bmdMode1440x900p60	= 0x77786736,
        bmdMode1440x1080p50	= 0x73786735,
        bmdMode1440x1080p60	= 0x73786736,
        bmdMode1600x1200p50	= 0x75786735,
        bmdMode1600x1200p60	= 0x75786736,
        bmdMode1920x1200p50	= 0x77757835,
        bmdMode1920x1200p60	= 0x77757836,
        bmdMode1920x1440p50	= 0x31393435,
        bmdMode1920x1440p60	= 0x31393436,
        bmdMode2560x1440p50	= 0x77716835,
        bmdMode2560x1440p60	= 0x77716836,
        bmdMode2560x1600p50	= 0x77717835,
        bmdMode2560x1600p60	= 0x77717836,
        bmdModeUnknown	= 0x69756e6b
    } 	BMDDisplayMode;

typedef /* [v1_enum] */ 
enum _BMDFieldDominance
    {
        bmdUnknownFieldDominance	= 0,
        bmdLowerFieldFirst	= 0x6c6f7772,
        bmdUpperFieldFirst	= 0x75707072,
        bmdProgressiveFrame	= 0x70726f67,
        bmdProgressiveSegmentedFrame	= 0x70736620
    } 	BMDFieldDominance;

typedef /* [v1_enum] */ 
enum _BMDPixelFormat
    {
        bmdFormatUnspecified	= 0,
        bmdFormat8BitYUV	= 0x32767579,
        bmdFormat10BitYUV	= 0x76323130,
        bmdFormat10BitYUVA	= 0x41793130,
        bmdFormat8BitARGB	= 32,
        bmdFormat8BitBGRA	= 0x42475241,
        bmdFormat10BitRGB	= 0x72323130,
        bmdFormat12BitRGB	= 0x52313242,
        bmdFormat12BitRGBLE	= 0x5231324c,
        bmdFormat10BitRGBXLE	= 0x5231306c,
        bmdFormat10BitRGBX	= 0x52313062,
        bmdFormatH265	= 0x68657631,
        bmdFormatDNxHR	= 0x41566468
    } 	BMDPixelFormat;

/* [v1_enum] */ 
enum _BMDDisplayModeFlags
    {
        bmdDisplayModeSupports3D	= ( 1 << 0 ) ,
        bmdDisplayModeColorspaceRec601	= ( 1 << 1 ) ,
        bmdDisplayModeColorspaceRec709	= ( 1 << 2 ) ,
        bmdDisplayModeColorspaceRec2020	= ( 1 << 3 ) 
    } ;


#if 0
#endif

#if 0
#endif
typedef /* [v1_enum] */ 
enum _BMDDeckLinkConfigurationID
    {
        bmdDeckLinkConfigSwapSerialRxTx	= 0x73737274,
        bmdDeckLinkConfigHDMI3DPackingFormat	= 0x33647066,
        bmdDeckLinkConfigBypass	= 0x62797073,
        bmdDeckLinkConfigClockTimingAdjustment	= 0x63746164,
        bmdDeckLinkConfigAnalogAudioConsumerLevels	= 0x6161636c,
        bmdDeckLinkConfigSwapHDMICh3AndCh4OnInput	= 0x68693334,
        bmdDeckLinkConfigSwapHDMICh3AndCh4OnOutput	= 0x686f3334,
        bmdDeckLinkConfigFieldFlickerRemoval	= 0x66646672,
        bmdDeckLinkConfigHD1080p24ToHD1080i5994Conversion	= 0x746f3539,
        bmdDeckLinkConfig444SDIVideoOutput	= 0x3434346f,
        bmdDeckLinkConfigBlackVideoOutputDuringCapture	= 0x62766f63,
        bmdDeckLinkConfigLowLatencyVideoOutput	= 0x6c6c766f,
        bmdDeckLinkConfigDownConversionOnAllAnalogOutput	= 0x6361616f,
        bmdDeckLinkConfigSMPTELevelAOutput	= 0x736d7461,
        bmdDeckLinkConfigRec2020Output	= 0x72656332,
        bmdDeckLinkConfigQuadLinkSDIVideoOutputSquareDivisionSplit	= 0x53445153,
        bmdDeckLinkConfigOutput1080pAsPsF	= 0x70667072,
        bmdDeckLinkConfigOutputValidateEDIDForDolbyVision	= 0x70726564,
        bmdDeckLinkConfigVideoOutputConnection	= 0x766f636e,
        bmdDeckLinkConfigVideoOutputConversionMode	= 0x766f636d,
        bmdDeckLinkConfigVideoOutputConversionColorspaceDestination	= 0x76636364,
        bmdDeckLinkConfigVideoOutputConversionColorspaceSource	= 0x76636373,
        bmdDeckLinkConfigAnalogVideoOutputFlags	= 0x61766f66,
        bmdDeckLinkConfigReferenceInputTimingOffset	= 0x676c6f74,
        bmdDeckLinkConfigReferenceOutputMode	= 0x676c4f6d,
        bmdDeckLinkConfigVideoOutputIdleOperation	= 0x766f696f,
        bmdDeckLinkConfigDefaultVideoOutputMode	= 0x64766f6d,
        bmdDeckLinkConfigDefaultVideoOutputModeFlags	= 0x64766f66,
        bmdDeckLinkConfigSDIOutputLinkConfiguration	= 0x736f6c63,
        bmdDeckLinkConfigHDMITimecodePacking	= 0x6874706b,
        bmdDeckLinkConfigPlaybackGroup	= 0x706c6772,
        bmdDeckLinkConfigVideoOutputComponentLumaGain	= 0x6f636c67,
        bmdDeckLinkConfigVideoOutputComponentChromaBlueGain	= 0x6f636362,
        bmdDeckLinkConfigVideoOutputComponentChromaRedGain	= 0x6f636372,
        bmdDeckLinkConfigVideoOutputCompositeLumaGain	= 0x6f696c67,
        bmdDeckLinkConfigVideoOutputCompositeChromaGain	= 0x6f696367,
        bmdDeckLinkConfigVideoOutputSVideoLumaGain	= 0x6f736c67,
        bmdDeckLinkConfigVideoOutputSVideoChromaGain	= 0x6f736367,
        bmdDeckLinkConfigDolbyVisionCMVersion	= 0x64767672,
        bmdDeckLinkConfigDolbyVisionMasterMinimumNits	= 0x6d6e6e74,
        bmdDeckLinkConfigDolbyVisionMasterMaximumNits	= 0x6d786e74,
        bmdDeckLinkConfigVideoInputScanning	= 0x76697363,
        bmdDeckLinkConfigUseDedicatedLTCInput	= 0x646c7463,
        bmdDeckLinkConfigSDIInput3DPayloadOverride	= 0x33646473,
        bmdDeckLinkConfigCapture1080pAsPsF	= 0x63667072,
        bmdDeckLinkConfigVideoInputConnection	= 0x7669636e,
        bmdDeckLinkConfigAnalogVideoInputFlags	= 0x61766966,
        bmdDeckLinkConfigVideoInputConversionMode	= 0x7669636d,
        bmdDeckLinkConfig32PulldownSequenceInitialTimecodeFrame	= 0x70646966,
        bmdDeckLinkConfigVANCSourceLine1Mapping	= 0x76736c31,
        bmdDeckLinkConfigVANCSourceLine2Mapping	= 0x76736c32,
        bmdDeckLinkConfigVANCSourceLine3Mapping	= 0x76736c33,
        bmdDeckLinkConfigCapturePassThroughMode	= 0x6370746d,
        bmdDeckLinkConfigCaptureGroup	= 0x63706772,
        bmdDeckLinkConfigVideoInputComponentLumaGain	= 0x69636c67,
        bmdDeckLinkConfigVideoInputComponentChromaBlueGain	= 0x69636362,
        bmdDeckLinkConfigVideoInputComponentChromaRedGain	= 0x69636372,
        bmdDeckLinkConfigVideoInputCompositeLumaGain	= 0x69696c67,
        bmdDeckLinkConfigVideoInputCompositeChromaGain	= 0x69696367,
        bmdDeckLinkConfigVideoInputSVideoLumaGain	= 0x69736c67,
        bmdDeckLinkConfigVideoInputSVideoChromaGain	= 0x69736367,
        bmdDeckLinkConfigInternalKeyingAncillaryDataSource	= 0x696b6173,
        bmdDeckLinkConfigMicrophonePhantomPower	= 0x6d706870,
        bmdDeckLinkConfigAudioInputConnection	= 0x6169636e,
        bmdDeckLinkConfigAnalogAudioInputScaleChannel1	= 0x61697331,
        bmdDeckLinkConfigAnalogAudioInputScaleChannel2	= 0x61697332,
        bmdDeckLinkConfigAnalogAudioInputScaleChannel3	= 0x61697333,
        bmdDeckLinkConfigAnalogAudioInputScaleChannel4	= 0x61697334,
        bmdDeckLinkConfigDigitalAudioInputScale	= 0x64616973,
        bmdDeckLinkConfigMicrophoneInputGain	= 0x6d696367,
        bmdDeckLinkConfigAudioOutputAESAnalogSwitch	= 0x616f6161,
        bmdDeckLinkConfigAnalogAudioOutputScaleChannel1	= 0x616f7331,
        bmdDeckLinkConfigAnalogAudioOutputScaleChannel2	= 0x616f7332,
        bmdDeckLinkConfigAnalogAudioOutputScaleChannel3	= 0x616f7333,
        bmdDeckLinkConfigAnalogAudioOutputScaleChannel4	= 0x616f7334,
        bmdDeckLinkConfigDigitalAudioOutputScale	= 0x64616f73,
        bmdDeckLinkConfigHeadphoneVolume	= 0x68766f6c,
        bmdDeckLinkConfigEthernetUseDHCP	= 0x44484350,
        bmdDeckLinkConfigEthernetPTPFollowerOnly	= 0x50545066,
        bmdDeckLinkConfigEthernetPTPUseUDPEncapsulation	= 0x50545055,
        bmdDeckLinkConfigEthernetPTPPriority1	= 0x50545031,
        bmdDeckLinkConfigEthernetPTPPriority2	= 0x50545032,
        bmdDeckLinkConfigEthernetPTPDomain	= 0x50545044,
        bmdDeckLinkConfigEthernetPTPLogAnnounceInterval	= 0x50545041,
        bmdDeckLinkConfigEthernetStaticLocalIPAddress	= 0x6e736970,
        bmdDeckLinkConfigEthernetStaticSubnetMask	= 0x6e73736d,
        bmdDeckLinkConfigEthernetStaticGatewayIPAddress	= 0x6e736777,
        bmdDeckLinkConfigEthernetStaticPrimaryDNS	= 0x6e737064,
        bmdDeckLinkConfigEthernetStaticSecondaryDNS	= 0x6e737364,
        bmdDeckLinkConfigEthernetVideoOutputAddress	= 0x6e6f6176,
        bmdDeckLinkConfigEthernetAudioOutputAddress	= 0x6e6f6161,
        bmdDeckLinkConfigEthernetAncillaryOutputAddress	= 0x6e6f6141,
        bmdDeckLinkConfigEthernetAudioOutputChannelOrder	= 0x6361636f,
        bmdDeckLinkConfigDeviceInformationLabel	= 0x64696c61,
        bmdDeckLinkConfigDeviceInformationSerialNumber	= 0x6469736e,
        bmdDeckLinkConfigDeviceInformationCompany	= 0x6469636f,
        bmdDeckLinkConfigDeviceInformationPhone	= 0x64697068,
        bmdDeckLinkConfigDeviceInformationEmail	= 0x6469656d,
        bmdDeckLinkConfigDeviceInformationDate	= 0x64696461,
        bmdDeckLinkConfigDeckControlConnection	= 0x6463636f
    } 	BMDDeckLinkConfigurationID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkEncoderConfigurationID
    {
        bmdDeckLinkEncoderConfigPreferredBitDepth	= 0x65706272,
        bmdDeckLinkEncoderConfigFrameCodingMode	= 0x6566636d,
        bmdDeckLinkEncoderConfigH265TargetBitrate	= 0x68746272,
        bmdDeckLinkEncoderConfigDNxHRCompressionID	= 0x64636964,
        bmdDeckLinkEncoderConfigDNxHRLevel	= 0x646c6576,
        bmdDeckLinkEncoderConfigMPEG4SampleDescription	= 0x73747345,
        bmdDeckLinkEncoderConfigMPEG4CodecSpecificDesc	= 0x65736473
    } 	BMDDeckLinkEncoderConfigurationID;



typedef unsigned int BMDDeckControlStatusFlags;
typedef unsigned int BMDDeckControlExportModeOpsFlags;
#if 0
typedef enum _BMDDeckControlStatusFlags BMDDeckControlStatusFlags;

typedef enum _BMDDeckControlExportModeOpsFlags BMDDeckControlExportModeOpsFlags;

#endif
typedef /* [v1_enum] */ 
enum _BMDDeckControlMode
    {
        bmdDeckControlNotOpened	= 0x6e746f70,
        bmdDeckControlVTRControlMode	= 0x76747263,
        bmdDeckControlExportMode	= 0x6578706d,
        bmdDeckControlCaptureMode	= 0x6361706d
    } 	BMDDeckControlMode;

typedef /* [v1_enum] */ 
enum _BMDDeckControlEvent
    {
        bmdDeckControlAbortedEvent	= 0x61627465,
        bmdDeckControlPrepareForExportEvent	= 0x70666565,
        bmdDeckControlExportCompleteEvent	= 0x65786365,
        bmdDeckControlPrepareForCaptureEvent	= 0x70666365,
        bmdDeckControlCaptureCompleteEvent	= 0x63636576
    } 	BMDDeckControlEvent;

typedef /* [v1_enum] */ 
enum _BMDDeckControlVTRControlState
    {
        bmdDeckControlNotInVTRControlMode	= 0x6e76636d,
        bmdDeckControlVTRControlPlaying	= 0x76747270,
        bmdDeckControlVTRControlRecording	= 0x76747272,
        bmdDeckControlVTRControlStill	= 0x76747261,
        bmdDeckControlVTRControlShuttleForward	= 0x76747366,
        bmdDeckControlVTRControlShuttleReverse	= 0x76747372,
        bmdDeckControlVTRControlJogForward	= 0x76746a66,
        bmdDeckControlVTRControlJogReverse	= 0x76746a72,
        bmdDeckControlVTRControlStopped	= 0x7674726f
    } 	BMDDeckControlVTRControlState;

/* [v1_enum] */ 
enum _BMDDeckControlStatusFlags
    {
        bmdDeckControlStatusDeckConnected	= ( 1 << 0 ) ,
        bmdDeckControlStatusRemoteMode	= ( 1 << 1 ) ,
        bmdDeckControlStatusRecordInhibited	= ( 1 << 2 ) ,
        bmdDeckControlStatusCassetteOut	= ( 1 << 3 ) 
    } ;
/* [v1_enum] */ 
enum _BMDDeckControlExportModeOpsFlags
    {
        bmdDeckControlExportModeInsertVideo	= ( 1 << 0 ) ,
        bmdDeckControlExportModeInsertAudio1	= ( 1 << 1 ) ,
        bmdDeckControlExportModeInsertAudio2	= ( 1 << 2 ) ,
        bmdDeckControlExportModeInsertAudio3	= ( 1 << 3 ) ,
        bmdDeckControlExportModeInsertAudio4	= ( 1 << 4 ) ,
        bmdDeckControlExportModeInsertAudio5	= ( 1 << 5 ) ,
        bmdDeckControlExportModeInsertAudio6	= ( 1 << 6 ) ,
        bmdDeckControlExportModeInsertAudio7	= ( 1 << 7 ) ,
        bmdDeckControlExportModeInsertAudio8	= ( 1 << 8 ) ,
        bmdDeckControlExportModeInsertAudio9	= ( 1 << 9 ) ,
        bmdDeckControlExportModeInsertAudio10	= ( 1 << 10 ) ,
        bmdDeckControlExportModeInsertAudio11	= ( 1 << 11 ) ,
        bmdDeckControlExportModeInsertAudio12	= ( 1 << 12 ) ,
        bmdDeckControlExportModeInsertTimeCode	= ( 1 << 13 ) ,
        bmdDeckControlExportModeInsertAssemble	= ( 1 << 14 ) ,
        bmdDeckControlExportModeInsertPreview	= ( 1 << 15 ) ,
        bmdDeckControlUseManualExport	= ( 1 << 16 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDDeckControlError
    {
        bmdDeckControlNoError	= 0x6e6f6572,
        bmdDeckControlModeError	= 0x6d6f6572,
        bmdDeckControlMissedInPointError	= 0x6d696572,
        bmdDeckControlDeckTimeoutError	= 0x64746572,
        bmdDeckControlCommandFailedError	= 0x63666572,
        bmdDeckControlDeviceAlreadyOpenedError	= 0x64616c6f,
        bmdDeckControlFailedToOpenDeviceError	= 0x66646572,
        bmdDeckControlInLocalModeError	= 0x6c6d6572,
        bmdDeckControlEndOfTapeError	= 0x65746572,
        bmdDeckControlUserAbortError	= 0x75616572,
        bmdDeckControlNoTapeInDeckError	= 0x6e746572,
        bmdDeckControlNoVideoFromCardError	= 0x6e766663,
        bmdDeckControlNoCommunicationError	= 0x6e636f6d,
        bmdDeckControlBufferTooSmallError	= 0x6274736d,
        bmdDeckControlBadChecksumError	= 0x63686b73,
        bmdDeckControlUnknownError	= 0x756e6572
    } 	BMDDeckControlError;



#if 0
#endif
typedef /* [v1_enum] */ 
enum _BMDStreamingDeviceMode
    {
        bmdStreamingDeviceIdle	= 0x69646c65,
        bmdStreamingDeviceEncoding	= 0x656e636f,
        bmdStreamingDeviceStopping	= 0x73746f70,
        bmdStreamingDeviceUnknown	= 0x6d756e6b
    } 	BMDStreamingDeviceMode;

typedef /* [v1_enum] */ 
enum _BMDStreamingEncodingFrameRate
    {
        bmdStreamingEncodedFrameRate50i	= 0x65353069,
        bmdStreamingEncodedFrameRate5994i	= 0x65353969,
        bmdStreamingEncodedFrameRate60i	= 0x65363069,
        bmdStreamingEncodedFrameRate2398p	= 0x65323370,
        bmdStreamingEncodedFrameRate24p	= 0x65323470,
        bmdStreamingEncodedFrameRate25p	= 0x65323570,
        bmdStreamingEncodedFrameRate2997p	= 0x65323970,
        bmdStreamingEncodedFrameRate30p	= 0x65333070,
        bmdStreamingEncodedFrameRate50p	= 0x65353070,
        bmdStreamingEncodedFrameRate5994p	= 0x65353970,
        bmdStreamingEncodedFrameRate60p	= 0x65363070
    } 	BMDStreamingEncodingFrameRate;

typedef /* [v1_enum] */ 
enum _BMDStreamingEncodingSupport
    {
        bmdStreamingEncodingModeNotSupported	= 0,
        bmdStreamingEncodingModeSupported	= ( bmdStreamingEncodingModeNotSupported + 1 ) ,
        bmdStreamingEncodingModeSupportedWithChanges	= ( bmdStreamingEncodingModeSupported + 1 ) 
    } 	BMDStreamingEncodingSupport;

typedef /* [v1_enum] */ 
enum _BMDStreamingVideoCodec
    {
        bmdStreamingVideoCodecH264	= 0x48323634
    } 	BMDStreamingVideoCodec;

typedef /* [v1_enum] */ 
enum _BMDStreamingH264Profile
    {
        bmdStreamingH264ProfileHigh	= 0x68696768,
        bmdStreamingH264ProfileMain	= 0x6d61696e,
        bmdStreamingH264ProfileBaseline	= 0x62617365
    } 	BMDStreamingH264Profile;

typedef /* [v1_enum] */ 
enum _BMDStreamingH264Level
    {
        bmdStreamingH264Level12	= 0x6c763132,
        bmdStreamingH264Level13	= 0x6c763133,
        bmdStreamingH264Level2	= 0x6c763220,
        bmdStreamingH264Level21	= 0x6c763231,
        bmdStreamingH264Level22	= 0x6c763232,
        bmdStreamingH264Level3	= 0x6c763320,
        bmdStreamingH264Level31	= 0x6c763331,
        bmdStreamingH264Level32	= 0x6c763332,
        bmdStreamingH264Level4	= 0x6c763420,
        bmdStreamingH264Level41	= 0x6c763431,
        bmdStreamingH264Level42	= 0x6c763432
    } 	BMDStreamingH264Level;

typedef /* [v1_enum] */ 
enum _BMDStreamingH264EntropyCoding
    {
        bmdStreamingH264EntropyCodingCAVLC	= 0x45564c43,
        bmdStreamingH264EntropyCodingCABAC	= 0x45424143
    } 	BMDStreamingH264EntropyCoding;

typedef /* [v1_enum] */ 
enum _BMDStreamingAudioCodec
    {
        bmdStreamingAudioCodecAAC	= 0x41414320
    } 	BMDStreamingAudioCodec;

typedef /* [v1_enum] */ 
enum _BMDStreamingEncodingModePropertyID
    {
        bmdStreamingEncodingPropertyVideoFrameRate	= 0x76667274,
        bmdStreamingEncodingPropertyVideoBitRateKbps	= 0x76627274,
        bmdStreamingEncodingPropertyH264Profile	= 0x68707266,
        bmdStreamingEncodingPropertyH264Level	= 0x686c766c,
        bmdStreamingEncodingPropertyH264EntropyCoding	= 0x68656e74,
        bmdStreamingEncodingPropertyH264HasBFrames	= 0x68426672,
        bmdStreamingEncodingPropertyAudioCodec	= 0x61636463,
        bmdStreamingEncodingPropertyAudioSampleRate	= 0x61737274,
        bmdStreamingEncodingPropertyAudioChannelCount	= 0x61636863,
        bmdStreamingEncodingPropertyAudioBitRateKbps	= 0x61627274
    } 	BMDStreamingEncodingModePropertyID;












typedef unsigned int BMDBufferAccessFlags;
typedef unsigned int BMDFrameFlags;
typedef unsigned int BMDVideoInputFlags;
typedef unsigned int BMDVideoInputFormatChangedEvents;
typedef unsigned int BMDDetectedVideoInputFormatFlags;
typedef unsigned int BMDDeckLinkCapturePassthroughMode;
typedef unsigned int BMDAnalogVideoFlags;
typedef unsigned int BMDFormatFlags;
typedef unsigned int BMDDeviceBusyState;
#if 0
typedef enum _BMDBufferAccessFlags BMDBufferAccessFlags;

typedef enum _BMDFrameFlags BMDFrameFlags;

typedef enum _BMDVideoInputFlags BMDVideoInputFlags;

typedef enum _BMDVideoInputFormatChangedEvents BMDVideoInputFormatChangedEvents;

typedef enum _BMDDetectedVideoInputFormatFlags BMDDetectedVideoInputFormatFlags;

typedef enum _BMDDeckLinkCapturePassthroughMode BMDDeckLinkCapturePassthroughMode;

typedef enum _BMDAnalogVideoFlags BMDAnalogVideoFlags;

typedef enum _BMDFormatFlags BMDFormatFlags;

typedef enum _BMDDeviceBusyState BMDDeviceBusyState;

#endif
/* [v1_enum] */ 
enum _BMDBufferAccessFlags
    {
        bmdBufferAccessReadAndWrite	= ( ( 1 << 0 )  | ( 1 << 1 )  ) ,
        bmdBufferAccessRead	= ( 1 << 0 ) ,
        bmdBufferAccessWrite	= ( 1 << 1 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDVideoOutputFlags
    {
        bmdVideoOutputFlagDefault	= 0,
        bmdVideoOutputVANC	= ( 1 << 0 ) ,
        bmdVideoOutputVITC	= ( 1 << 1 ) ,
        bmdVideoOutputRP188	= ( 1 << 2 ) ,
        bmdVideoOutputDualStream3D	= ( 1 << 4 ) ,
        bmdVideoOutputSynchronizeToPlaybackGroup	= ( 1 << 6 ) ,
        bmdVideoOutputDolbyVision	= ( 1 << 7 ) 
    } 	BMDVideoOutputFlags;

typedef /* [v1_enum] */ 
enum _BMDSupportedVideoModeFlags
    {
        bmdSupportedVideoModeDefault	= 0,
        bmdSupportedVideoModeKeying	= ( 1 << 0 ) ,
        bmdSupportedVideoModeDualStream3D	= ( 1 << 1 ) ,
        bmdSupportedVideoModeSDISingleLink	= ( 1 << 2 ) ,
        bmdSupportedVideoModeSDIDualLink	= ( 1 << 3 ) ,
        bmdSupportedVideoModeSDIQuadLink	= ( 1 << 4 ) ,
        bmdSupportedVideoModeInAnyProfile	= ( 1 << 5 ) ,
        bmdSupportedVideoModePsF	= ( 1 << 6 ) ,
        bmdSupportedVideoModeDolbyVision	= ( 1 << 7 ) 
    } 	BMDSupportedVideoModeFlags;

typedef /* [v1_enum] */ 
enum _BMDPacketType
    {
        bmdPacketTypeStreamInterruptedMarker	= 0x73696e74,
        bmdPacketTypeStreamData	= 0x73646174
    } 	BMDPacketType;

/* [v1_enum] */ 
enum _BMDFrameFlags
    {
        bmdFrameFlagDefault	= 0,
        bmdFrameFlagFlipVertical	= ( 1 << 0 ) ,
        bmdFrameFlagMonitorOutOnly	= ( 1 << 3 ) ,
        bmdFrameContainsHDRMetadata	= ( 1 << 1 ) ,
        bmdFrameContainsDolbyVisionMetadata	= ( 1 << 4 ) ,
        bmdFrameCapturedAsPsF	= ( 1 << 30 ) ,
        bmdFrameHasNoInputSource	= ( 1 << 31 ) 
    } ;
/* [v1_enum] */ 
enum _BMDVideoInputFlags
    {
        bmdVideoInputFlagDefault	= 0,
        bmdVideoInputEnableFormatDetection	= ( 1 << 0 ) ,
        bmdVideoInputDualStream3D	= ( 1 << 1 ) ,
        bmdVideoInputSynchronizeToCaptureGroup	= ( 1 << 2 ) 
    } ;
/* [v1_enum] */ 
enum _BMDVideoInputFormatChangedEvents
    {
        bmdVideoInputDisplayModeChanged	= ( 1 << 0 ) ,
        bmdVideoInputFieldDominanceChanged	= ( 1 << 1 ) ,
        bmdVideoInputColorspaceChanged	= ( 1 << 2 ) 
    } ;
/* [v1_enum] */ 
enum _BMDDetectedVideoInputFormatFlags
    {
        bmdDetectedVideoInputYCbCr422	= ( 1 << 0 ) ,
        bmdDetectedVideoInputRGB444	= ( 1 << 1 ) ,
        bmdDetectedVideoInputDualStream3D	= ( 1 << 2 ) ,
        bmdDetectedVideoInput12BitDepth	= ( 1 << 3 ) ,
        bmdDetectedVideoInput10BitDepth	= ( 1 << 4 ) ,
        bmdDetectedVideoInput8BitDepth	= ( 1 << 5 ) 
    } ;
/* [v1_enum] */ 
enum _BMDDeckLinkCapturePassthroughMode
    {
        bmdDeckLinkCapturePassthroughModeDisabled	= 0x70646973,
        bmdDeckLinkCapturePassthroughModeDirect	= 0x70646972,
        bmdDeckLinkCapturePassthroughModeCleanSwitch	= 0x70636c6e
    } ;
typedef /* [v1_enum] */ 
enum _BMDOutputFrameCompletionResult
    {
        bmdOutputFrameCompleted	= 0,
        bmdOutputFrameDisplayedLate	= ( bmdOutputFrameCompleted + 1 ) ,
        bmdOutputFrameDropped	= ( bmdOutputFrameDisplayedLate + 1 ) ,
        bmdOutputFrameFlushed	= ( bmdOutputFrameDropped + 1 ) 
    } 	BMDOutputFrameCompletionResult;

typedef /* [v1_enum] */ 
enum _BMDReferenceStatus
    {
        bmdReferenceUnlocked	= 0,
        bmdReferenceNotSupportedByHardware	= ( 1 << 0 ) ,
        bmdReferenceLocked	= ( 1 << 1 ) 
    } 	BMDReferenceStatus;

typedef /* [v1_enum] */ 
enum _BMDAudioFormat
    {
        bmdAudioFormatPCM	= 0x6c70636d
    } 	BMDAudioFormat;

typedef /* [v1_enum] */ 
enum _BMDAudioSampleRate
    {
        bmdAudioSampleRate48kHz	= 48000
    } 	BMDAudioSampleRate;

typedef /* [v1_enum] */ 
enum _BMDAudioSampleType
    {
        bmdAudioSampleType16bitInteger	= 16,
        bmdAudioSampleType32bitInteger	= 32
    } 	BMDAudioSampleType;

typedef /* [v1_enum] */ 
enum _BMDAudioOutputStreamType
    {
        bmdAudioOutputStreamContinuous	= 0,
        bmdAudioOutputStreamContinuousDontResample	= ( bmdAudioOutputStreamContinuous + 1 ) ,
        bmdAudioOutputStreamTimestamped	= ( bmdAudioOutputStreamContinuousDontResample + 1 ) 
    } 	BMDAudioOutputStreamType;

typedef /* [v1_enum] */ 
enum _BMDAncillaryPacketFormat
    {
        bmdAncillaryPacketFormatUInt8	= 0x75693038,
        bmdAncillaryPacketFormatUInt16	= 0x75693136,
        bmdAncillaryPacketFormatYCbCr10	= 0x76323130
    } 	BMDAncillaryPacketFormat;

typedef /* [v1_enum] */ 
enum _BMDTimecodeFormat
    {
        bmdTimecodeRP188VITC1	= 0x72707631,
        bmdTimecodeRP188VITC2	= 0x72703132,
        bmdTimecodeRP188LTC	= 0x72706c74,
        bmdTimecodeRP188HighFrameRate	= 0x72706872,
        bmdTimecodeRP188Any	= 0x72703138,
        bmdTimecodeVITC	= 0x76697463,
        bmdTimecodeVITCField2	= 0x76697432,
        bmdTimecodeSerial	= 0x73657269
    } 	BMDTimecodeFormat;

/* [v1_enum] */ 
enum _BMDAnalogVideoFlags
    {
        bmdAnalogVideoFlagCompositeSetup75	= ( 1 << 0 ) ,
        bmdAnalogVideoFlagComponentBetacamLevels	= ( 1 << 1 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDAudioOutputAnalogAESSwitch
    {
        bmdAudioOutputSwitchAESEBU	= 0x61657320,
        bmdAudioOutputSwitchAnalog	= 0x616e6c67
    } 	BMDAudioOutputAnalogAESSwitch;

typedef /* [v1_enum] */ 
enum _BMDVideoOutputConversionMode
    {
        bmdNoVideoOutputConversion	= 0x6e6f6e65,
        bmdVideoOutputLetterboxDownconversion	= 0x6c746278,
        bmdVideoOutputAnamorphicDownconversion	= 0x616d7068,
        bmdVideoOutputHD720toHD1080Conversion	= 0x37323063,
        bmdVideoOutputHardwareLetterboxDownconversion	= 0x48576c62,
        bmdVideoOutputHardwareAnamorphicDownconversion	= 0x4857616d,
        bmdVideoOutputHardwareCenterCutDownconversion	= 0x48576363,
        bmdVideoOutputHardware720p1080pCrossconversion	= 0x78636170,
        bmdVideoOutputHardwareAnamorphic720pUpconversion	= 0x75613770,
        bmdVideoOutputHardwareAnamorphic1080iUpconversion	= 0x75613169,
        bmdVideoOutputHardwareAnamorphic149To720pUpconversion	= 0x75343770,
        bmdVideoOutputHardwareAnamorphic149To1080iUpconversion	= 0x75343169,
        bmdVideoOutputHardwarePillarbox720pUpconversion	= 0x75703770,
        bmdVideoOutputHardwarePillarbox1080iUpconversion	= 0x75703169
    } 	BMDVideoOutputConversionMode;

typedef /* [v1_enum] */ 
enum _BMDVideoInputConversionMode
    {
        bmdNoVideoInputConversion	= 0x6e6f6e65,
        bmdVideoInputLetterboxDownconversionFromHD1080	= 0x31306c62,
        bmdVideoInputAnamorphicDownconversionFromHD1080	= 0x3130616d,
        bmdVideoInputLetterboxDownconversionFromHD720	= 0x37326c62,
        bmdVideoInputAnamorphicDownconversionFromHD720	= 0x3732616d,
        bmdVideoInputLetterboxUpconversion	= 0x6c627570,
        bmdVideoInputAnamorphicUpconversion	= 0x616d7570
    } 	BMDVideoInputConversionMode;

typedef /* [v1_enum] */ 
enum _BMDVideo3DPackingFormat
    {
        bmdVideo3DPackingSidebySideHalf	= 0x73627368,
        bmdVideo3DPackingLinebyLine	= 0x6c62796c,
        bmdVideo3DPackingTopAndBottom	= 0x7461626f,
        bmdVideo3DPackingFramePacking	= 0x6672706b,
        bmdVideo3DPackingLeftOnly	= 0x6c656674,
        bmdVideo3DPackingRightOnly	= 0x72696768
    } 	BMDVideo3DPackingFormat;

typedef /* [v1_enum] */ 
enum _BMDIdleVideoOutputOperation
    {
        bmdIdleVideoOutputBlack	= 0x626c6163,
        bmdIdleVideoOutputLastFrame	= 0x6c616661
    } 	BMDIdleVideoOutputOperation;

typedef /* [v1_enum] */ 
enum _BMDVideoEncoderFrameCodingMode
    {
        bmdVideoEncoderFrameCodingModeInter	= 0x696e7465,
        bmdVideoEncoderFrameCodingModeIntra	= 0x696e7472
    } 	BMDVideoEncoderFrameCodingMode;

typedef /* [v1_enum] */ 
enum _BMDDNxHRLevel
    {
        bmdDNxHRLevelSQ	= 0x646e7371,
        bmdDNxHRLevelLB	= 0x646e6c62,
        bmdDNxHRLevelHQ	= 0x646e6871,
        bmdDNxHRLevelHQX	= 0x64687178,
        bmdDNxHRLevel444	= 0x64343434
    } 	BMDDNxHRLevel;

typedef /* [v1_enum] */ 
enum _BMDLinkConfiguration
    {
        bmdLinkConfigurationSingleLink	= 0x6c63736c,
        bmdLinkConfigurationDualLink	= 0x6c63646c,
        bmdLinkConfigurationQuadLink	= 0x6c63716c
    } 	BMDLinkConfiguration;

typedef /* [v1_enum] */ 
enum _BMDDeviceInterface
    {
        bmdDeviceInterfacePCI	= 0x70636920,
        bmdDeviceInterfaceUSB	= 0x75736220,
        bmdDeviceInterfaceThunderbolt	= 0x7468756e
    } 	BMDDeviceInterface;

typedef /* [v1_enum] */ 
enum _BMDColorspace
    {
        bmdColorspaceRec601	= 0x72363031,
        bmdColorspaceRec709	= 0x72373039,
        bmdColorspaceRec2020	= 0x32303230,
        bmdColorspaceDolbyVisionNative	= 0x446f5669,
        bmdColorspaceP3D65	= 0x50334436,
        bmdColorspaceUnknown	= 0x4e636f6c
    } 	BMDColorspace;

typedef /* [v1_enum] */ 
enum _BMDDynamicRange
    {
        bmdDynamicRangeSDR	= 0,
        bmdDynamicRangeHDRStaticPQ	= ( 1 << 29 ) ,
        bmdDynamicRangeHDRStaticHLG	= ( 1 << 30 ) 
    } 	BMDDynamicRange;

typedef /* [v1_enum] */ 
enum _BMDMezzanineType
    {
        bmdMezzanineTypeNone	= 0,
        bmdMezzanineTypeHDMI14OpticalSDI	= 0x6d7a6131,
        bmdMezzanineTypeQuadSDI	= 0x6d7a3473,
        bmdMezzanineTypeHDMI20OpticalSDI	= 0x6d7a6132,
        bmdMezzanineTypeHDMI21RS422	= 0x6d7a6872
    } 	BMDMezzanineType;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkHDMIInputEDIDID
    {
        bmdDeckLinkHDMIInputEDIDDynamicRange	= 0x48494479
    } 	BMDDeckLinkHDMIInputEDIDID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkFrameMetadataID
    {
        bmdDeckLinkFrameMetadataColorspace	= 0x63737063,
        bmdDeckLinkFrameMetadataHDRElectroOpticalTransferFunc	= 0x656f7466,
        bmdDeckLinkFrameMetadataDolbyVision	= 0x646f7669,
        bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedX	= 0x68647278,
        bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedY	= 0x68647279,
        bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenX	= 0x68646778,
        bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenY	= 0x68646779,
        bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueX	= 0x68646278,
        bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueY	= 0x68646279,
        bmdDeckLinkFrameMetadataHDRWhitePointX	= 0x68647778,
        bmdDeckLinkFrameMetadataHDRWhitePointY	= 0x68647779,
        bmdDeckLinkFrameMetadataHDRMaxDisplayMasteringLuminance	= 0x68646d6c,
        bmdDeckLinkFrameMetadataHDRMinDisplayMasteringLuminance	= 0x686d696c,
        bmdDeckLinkFrameMetadataHDRMaximumContentLightLevel	= 0x6d636c6c,
        bmdDeckLinkFrameMetadataHDRMaximumFrameAverageLightLevel	= 0x66616c6c
    } 	BMDDeckLinkFrameMetadataID;

typedef /* [v1_enum] */ 
enum _BMDEthernetLinkState
    {
        bmdEthernetLinkStateDisconnected	= 0x656c6473,
        bmdEthernetLinkStateConnectedUnbound	= 0x656c6375,
        bmdEthernetLinkStateConnectedBound	= 0x656c6362
    } 	BMDEthernetLinkState;

typedef /* [v1_enum] */ 
enum _BMDProfileID
    {
        bmdProfileOneSubDeviceFullDuplex	= 0x31646664,
        bmdProfileOneSubDeviceHalfDuplex	= 0x31646864,
        bmdProfileTwoSubDevicesFullDuplex	= 0x32646664,
        bmdProfileTwoSubDevicesHalfDuplex	= 0x32646864,
        bmdProfileFourSubDevicesHalfDuplex	= 0x34646864
    } 	BMDProfileID;

typedef /* [v1_enum] */ 
enum _BMDHDMITimecodePacking
    {
        bmdHDMITimecodePackingIEEEOUI000085	= 0x8500,
        bmdHDMITimecodePackingIEEEOUI080046	= 0x8004601,
        bmdHDMITimecodePackingIEEEOUI5CF9F0	= 0x5cf9f003
    } 	BMDHDMITimecodePacking;

typedef /* [v1_enum] */ 
enum _BMDInternalKeyingAncillaryDataSource
    {
        bmdInternalKeyingUsesAncillaryDataFromInputSignal	= 0x696b6169,
        bmdInternalKeyingUsesAncillaryDataFromKeyFrame	= 0x696b616b
    } 	BMDInternalKeyingAncillaryDataSource;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkAttributeID
    {
        BMDDeckLinkSupportsInternalKeying	= 0x6b657969,
        BMDDeckLinkSupportsExternalKeying	= 0x6b657965,
        BMDDeckLinkSupportsInputFormatDetection	= 0x696e6664,
        BMDDeckLinkHasReferenceInput	= 0x6872696e,
        BMDDeckLinkHasSerialPort	= 0x68737074,
        BMDDeckLinkHasAnalogVideoOutputGain	= 0x61766f67,
        BMDDeckLinkCanOnlyAdjustOverallVideoOutputGain	= 0x6f766f67,
        BMDDeckLinkHasVideoInputAntiAliasingFilter	= 0x6161666c,
        BMDDeckLinkHasBypass	= 0x62797073,
        BMDDeckLinkSupportsClockTimingAdjustment	= 0x63746164,
        BMDDeckLinkSupportsFullFrameReferenceInputTimingOffset	= 0x6672696e,
        BMDDeckLinkSupportsSMPTELevelAOutput	= 0x6c766c61,
        BMDDeckLinkSupportsAutoSwitchingPPsFOnInput	= 0x61707366,
        BMDDeckLinkSupportsDualLinkSDI	= 0x73646c73,
        BMDDeckLinkSupportsQuadLinkSDI	= 0x73716c73,
        BMDDeckLinkSupportsIdleOutput	= 0x69646f75,
        BMDDeckLinkVANCRequires10BitYUVVideoFrames	= 0x76696f59,
        BMDDeckLinkHasLTCTimecodeInput	= 0x686c7463,
        BMDDeckLinkSupportsHDRMetadata	= 0x6864726d,
        BMDDeckLinkSupportsColorspaceMetadata	= 0x636d6574,
        BMDDeckLinkSupportsHDMITimecode	= 0x6874696d,
        BMDDeckLinkSupportsHighFrameRateTimecode	= 0x48465254,
        BMDDeckLinkSupportsSynchronizeToCaptureGroup	= 0x73746367,
        BMDDeckLinkSupportsSynchronizeToPlaybackGroup	= 0x73747067,
        BMDDeckLinkHasMonitorOut	= 0x666d6f6f,
        BMDDeckLinkMaximumAudioChannels	= 0x6d616368,
        BMDDeckLinkMaximumHDMIAudioChannels	= 0x6d686368,
        BMDDeckLinkMaximumAnalogAudioInputChannels	= 0x69616368,
        BMDDeckLinkMaximumAnalogAudioOutputChannels	= 0x61616368,
        BMDDeckLinkNumberOfSubDevices	= 0x6e736264,
        BMDDeckLinkSubDeviceIndex	= 0x73756269,
        BMDDeckLinkPersistentID	= 0x70656964,
        BMDDeckLinkDeviceGroupID	= 0x64676964,
        BMDDeckLinkTopologicalID	= 0x746f6964,
        BMDDeckLinkVideoOutputConnections	= 0x766f636e,
        BMDDeckLinkVideoInputConnections	= 0x7669636e,
        BMDDeckLinkAudioOutputConnections	= 0x616f636e,
        BMDDeckLinkAudioInputConnections	= 0x6169636e,
        BMDDeckLinkVideoIOSupport	= 0x76696f73,
        BMDDeckLinkDeckControlConnections	= 0x6463636e,
        BMDDeckLinkDeviceInterface	= 0x64627573,
        BMDDeckLinkAudioInputRCAChannelCount	= 0x61697263,
        BMDDeckLinkAudioInputXLRChannelCount	= 0x61697863,
        BMDDeckLinkAudioOutputRCAChannelCount	= 0x616f7263,
        BMDDeckLinkAudioOutputXLRChannelCount	= 0x616f7863,
        BMDDeckLinkProfileID	= 0x70726964,
        BMDDeckLinkDuplex	= 0x64757078,
        BMDDeckLinkMinimumPrerollFrames	= 0x6d707266,
        BMDDeckLinkSupportedDynamicRange	= 0x73756472,
        BMDDeckLinkMezzanineType	= 0x6d657a74,
        BMDDeckLinkVideoInputGainMinimum	= 0x7669676d,
        BMDDeckLinkVideoInputGainMaximum	= 0x76696778,
        BMDDeckLinkVideoOutputGainMinimum	= 0x766f676d,
        BMDDeckLinkVideoOutputGainMaximum	= 0x766f6778,
        BMDDeckLinkMicrophoneInputGainMinimum	= 0x6d69676d,
        BMDDeckLinkMicrophoneInputGainMaximum	= 0x6d696778,
        BMDDeckLinkSerialPortDeviceName	= 0x736c706e,
        BMDDeckLinkVendorName	= 0x766e6472,
        BMDDeckLinkDisplayName	= 0x6473706e,
        BMDDeckLinkModelName	= 0x6d646c6e,
        BMDDeckLinkDeviceHandle	= 0x64657668,
        BMDDeckLinkEthernetMACAddress	= 0x654d4143
    } 	BMDDeckLinkAttributeID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkAPIInformationID
    {
        BMDDeckLinkAPIVersion	= 0x76657273
    } 	BMDDeckLinkAPIInformationID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkStatusID
    {
        bmdDeckLinkStatusDetectedVideoInputMode	= 0x6476696d,
        bmdDeckLinkStatusDetectedVideoInputFormatFlags	= 0x64766666,
        bmdDeckLinkStatusDetectedVideoInputFieldDominance	= 0x64766664,
        bmdDeckLinkStatusDetectedVideoInputColorspace	= 0x6473636c,
        bmdDeckLinkStatusDetectedVideoInputDynamicRange	= 0x64736472,
        bmdDeckLinkStatusDetectedSDILinkConfiguration	= 0x64736c63,
        bmdDeckLinkStatusCurrentVideoInputMode	= 0x6376696d,
        bmdDeckLinkStatusCurrentVideoInputPixelFormat	= 0x63766970,
        bmdDeckLinkStatusCurrentVideoInputFlags	= 0x63766966,
        bmdDeckLinkStatusCurrentVideoOutputMode	= 0x63766f6d,
        bmdDeckLinkStatusCurrentVideoOutputFlags	= 0x63766f66,
        bmdDeckLinkStatusEthernetLink	= 0x73656c73,
        bmdDeckLinkStatusEthernetLinkMbps	= 0x73657370,
        bmdDeckLinkStatusPCIExpressLinkWidth	= 0x70776964,
        bmdDeckLinkStatusPCIExpressLinkSpeed	= 0x706c6e6b,
        bmdDeckLinkStatusLastVideoOutputPixelFormat	= 0x6f706978,
        bmdDeckLinkStatusReferenceSignalMode	= 0x7265666d,
        bmdDeckLinkStatusReferenceSignalFlags	= 0x72656666,
        bmdDeckLinkStatusBusy	= 0x62757379,
        bmdDeckLinkStatusInterchangeablePanelType	= 0x69637074,
        bmdDeckLinkStatusDeviceTemperature	= 0x64746d70,
        bmdDeckLinkStatusHDMIOutputActualMode	= 0x6869616d,
        bmdDeckLinkStatusHDMIOutputActualFormatFlags	= 0x68696166,
        bmdDeckLinkStatusHDMIOutputFRLRate	= 0x68696f66,
        bmdDeckLinkStatusHDMIInputFRLRate	= 0x68696966,
        bmdDeckLinkStatusHDMIOutputTMDSLineRate	= 0x68696c72,
        bmdDeckLinkStatusSinkSupportsDolbyVision	= 0x64767672,
        bmdDeckLinkStatusVideoInputSignalLocked	= 0x7669736c,
        bmdDeckLinkStatusReferenceSignalLocked	= 0x7265666c,
        bmdDeckLinkStatusEthernetLocalIPAddress	= 0x73656970,
        bmdDeckLinkStatusEthernetSubnetMask	= 0x7365736d,
        bmdDeckLinkStatusEthernetGatewayIPAddress	= 0x73656777,
        bmdDeckLinkStatusEthernetPrimaryDNS	= 0x73657064,
        bmdDeckLinkStatusEthernetSecondaryDNS	= 0x73657364,
        bmdDeckLinkStatusEthernetPTPGrandmasterIdentity	= 0x73706964,
        bmdDeckLinkStatusEthernetVideoOutputAddress	= 0x736f6176,
        bmdDeckLinkStatusEthernetAudioOutputAddress	= 0x736f6161,
        bmdDeckLinkStatusEthernetAncillaryOutputAddress	= 0x736f6141,
        bmdDeckLinkStatusEthernetAudioInputChannelOrder	= 0x7361636f,
        bmdDeckLinkStatusReceivedEDID	= 0x65646964
    } 	BMDDeckLinkStatusID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkVideoStatusFlags
    {
        bmdDeckLinkVideoStatusPsF	= ( 1 << 0 ) ,
        bmdDeckLinkVideoStatusDualStream3D	= ( 1 << 1 ) 
    } 	BMDDeckLinkVideoStatusFlags;

typedef /* [v1_enum] */ 
enum _BMDDuplexMode
    {
        bmdDuplexFull	= 0x64786675,
        bmdDuplexHalf	= 0x64786861,
        bmdDuplexSimplex	= 0x64787370,
        bmdDuplexInactive	= 0x6478696e
    } 	BMDDuplexMode;

typedef /* [v1_enum] */ 
enum _BMDPanelType
    {
        bmdPanelNotDetected	= 0x6e706e6c,
        bmdPanelTeranexMiniSmartPanel	= 0x746d736d
    } 	BMDPanelType;

/* [v1_enum] */ 
enum _BMDFormatFlags
    {
        bmdFormatRGB444	= ( 1 << 0 ) ,
        bmdFormatYUV444	= ( 1 << 1 ) ,
        bmdFormatYUV422	= ( 1 << 2 ) ,
        bmdFormatYUV420	= ( 1 << 3 ) ,
        bmdFormat8BitDepth	= ( 1 << 4 ) ,
        bmdFormat10BitDepth	= ( 1 << 5 ) ,
        bmdFormat12BitDepth	= ( 1 << 6 ) 
    } ;
/* [v1_enum] */ 
enum _BMDDeviceBusyState
    {
        bmdDeviceCaptureBusy	= ( 1 << 0 ) ,
        bmdDevicePlaybackBusy	= ( 1 << 1 ) ,
        bmdDeviceSerialPortBusy	= ( 1 << 2 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDVideoIOSupport
    {
        bmdDeviceSupportsCapture	= ( 1 << 0 ) ,
        bmdDeviceSupportsPlayback	= ( 1 << 1 ) 
    } 	BMDVideoIOSupport;

typedef /* [v1_enum] */ 
enum _BMD3DPreviewFormat
    {
        bmd3DPreviewFormatDefault	= 0x64656661,
        bmd3DPreviewFormatLeftOnly	= 0x6c656674,
        bmd3DPreviewFormatRightOnly	= 0x72696768,
        bmd3DPreviewFormatSideBySide	= 0x73696465,
        bmd3DPreviewFormatTopBottom	= 0x746f7062
    } 	BMD3DPreviewFormat;

typedef /* [v1_enum] */ 
enum _BMDIPFlowDirection
    {
        bmdDeckLinkIPFlowDirectionOutput	= 0,
        bmdDeckLinkIPFlowDirectionInput	= 1
    } 	BMDIPFlowDirection;

typedef /* [v1_enum] */ 
enum _BMDIPFlowType
    {
        bmdDeckLinkIPFlowTypeVideo	= 0,
        bmdDeckLinkIPFlowTypeAudio	= 1,
        bmdDeckLinkIPFlowTypeAncillary	= 2
    } 	BMDIPFlowType;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkIPFlowAttributeID
    {
        bmdDeckLinkIPFlowID	= 0x32666169,
        bmdDeckLinkIPFlowDirection	= 0x32666164,
        bmdDeckLinkIPFlowType	= 0x32666174
    } 	BMDDeckLinkIPFlowAttributeID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkIPFlowStatusID
    {
        bmdDeckLinkIPFlowSDP	= 0x32666173
    } 	BMDDeckLinkIPFlowStatusID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkIPFlowSettingID
    {
        bmdDeckLinkIPFlowPeerSDP	= 0x32667073
    } 	BMDDeckLinkIPFlowSettingID;

typedef /* [v1_enum] */ 
enum _BMDNotifications
    {
        bmdPreferencesChanged	= 0x70726566,
        bmdStatusChanged	= 0x73746174,
        bmdIPFlowStatusChanged	= 0x62667363,
        bmdIPFlowSettingChanged	= 0x62666363
    } 	BMDNotifications;



































































typedef /* [v1_enum] */ 
enum _BMDDeckLinkStatusID_v11_5_1
    {
        bmdDeckLinkStatusDetectedVideoInputFlags_v11_5_1	= 0x64766966
    } 	BMDDeckLinkStatusID_v11_5_1;

typedef /* [v1_enum] */ 
enum _BMDDisplayModeSupport_v10_11
    {
        bmdDisplayModeNotSupported_v10_11	= 0,
        bmdDisplayModeSupported_v10_11	= ( bmdDisplayModeNotSupported_v10_11 + 1 ) ,
        bmdDisplayModeSupportedWithConversion_v10_11	= ( bmdDisplayModeSupported_v10_11 + 1 ) 
    } 	BMDDisplayModeSupport_v10_11;

typedef /* [v1_enum] */ 
enum _BMDDuplexMode_v10_11
    {
        bmdDuplexModeFull_v10_11	= 0x66647570,
        bmdDuplexModeHalf_v10_11	= 0x68647570
    } 	BMDDuplexMode_v10_11;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkConfigurationID_v10_11
    {
        bmdDeckLinkConfigDuplexMode_v10_11	= 0x64757078
    } 	BMDDeckLinkConfigurationID_v10_11;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkAttributeID_v10_11
    {
        BMDDeckLinkSupportsDuplexModeConfiguration_v10_11	= 0x64757078,
        BMDDeckLinkSupportsHDKeying_v10_11	= 0x6b657968,
        BMDDeckLinkPairedDevicePersistentID_v10_11	= 0x70706964,
        BMDDeckLinkSupportsFullDuplex_v10_11	= 0x66647570
    } 	BMDDeckLinkAttributeID_v10_11;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkStatusID_v10_11
    {
        bmdDeckLinkStatusDuplexMode_v10_11	= 0x64757078
    } 	BMDDeckLinkStatusID_v10_11;

typedef /* [v1_enum] */ 
enum _BMDDuplexStatus_v10_11
    {
        bmdDuplexFullDuplex_v10_11	= 0x66647570,
        bmdDuplexHalfDuplex_v10_11	= 0x68647570,
        bmdDuplexSimplex_v10_11	= 0x73706c78,
        bmdDuplexInactive_v10_11	= 0x696e6163
    } 	BMDDuplexStatus_v10_11;




typedef /* [v1_enum] */ 
enum _BMDDeckLinkConfigurationID_v10_9
    {
        bmdDeckLinkConfig1080pNotPsF_v10_9	= 0x6670726f
    } 	BMDDeckLinkConfigurationID_v10_9;


typedef /* [v1_enum] */ 
enum _BMDDeckLinkConfigurationID_v10_4
    {
        bmdDeckLinkConfigSingleLinkVideoOutput_v10_4	= 0x73676c6f
    } 	BMDDeckLinkConfigurationID_v10_4;


typedef /* [v1_enum] */ 
enum _BMDDeckLinkConfigurationID_v10_2
    {
        bmdDeckLinkConfig3GBpsVideoOutput_v10_2	= 0x33676273
    } 	BMDDeckLinkConfigurationID_v10_2;

typedef /* [v1_enum] */ 
enum _BMDAudioConnection_v10_2
    {
        bmdAudioConnectionEmbedded_v10_2	= 0x656d6264,
        bmdAudioConnectionAESEBU_v10_2	= 0x61657320,
        bmdAudioConnectionAnalog_v10_2	= 0x616e6c67,
        bmdAudioConnectionAnalogXLR_v10_2	= 0x61786c72,
        bmdAudioConnectionAnalogRCA_v10_2	= 0x61726361
    } 	BMDAudioConnection_v10_2;


typedef /* [v1_enum] */ 
enum _BMDDeckLinkFrameMetadataID_v11_5
    {
        bmdDeckLinkFrameMetadataCintelFilmType_v11_5	= 0x63667479,
        bmdDeckLinkFrameMetadataCintelFilmGauge_v11_5	= 0x63666761,
        bmdDeckLinkFrameMetadataCintelKeykodeLow_v11_5	= 0x636b6b6c,
        bmdDeckLinkFrameMetadataCintelKeykodeHigh_v11_5	= 0x636b6b68,
        bmdDeckLinkFrameMetadataCintelTile1Size_v11_5	= 0x63743173,
        bmdDeckLinkFrameMetadataCintelTile2Size_v11_5	= 0x63743273,
        bmdDeckLinkFrameMetadataCintelTile3Size_v11_5	= 0x63743373,
        bmdDeckLinkFrameMetadataCintelTile4Size_v11_5	= 0x63743473,
        bmdDeckLinkFrameMetadataCintelImageWidth_v11_5	= 0x49575078,
        bmdDeckLinkFrameMetadataCintelImageHeight_v11_5	= 0x49485078,
        bmdDeckLinkFrameMetadataCintelLinearMaskingRedInRed_v11_5	= 0x6d726972,
        bmdDeckLinkFrameMetadataCintelLinearMaskingGreenInRed_v11_5	= 0x6d676972,
        bmdDeckLinkFrameMetadataCintelLinearMaskingBlueInRed_v11_5	= 0x6d626972,
        bmdDeckLinkFrameMetadataCintelLinearMaskingRedInGreen_v11_5	= 0x6d726967,
        bmdDeckLinkFrameMetadataCintelLinearMaskingGreenInGreen_v11_5	= 0x6d676967,
        bmdDeckLinkFrameMetadataCintelLinearMaskingBlueInGreen_v11_5	= 0x6d626967,
        bmdDeckLinkFrameMetadataCintelLinearMaskingRedInBlue_v11_5	= 0x6d726962,
        bmdDeckLinkFrameMetadataCintelLinearMaskingGreenInBlue_v11_5	= 0x6d676962,
        bmdDeckLinkFrameMetadataCintelLinearMaskingBlueInBlue_v11_5	= 0x6d626962,
        bmdDeckLinkFrameMetadataCintelLogMaskingRedInRed_v11_5	= 0x6d6c7272,
        bmdDeckLinkFrameMetadataCintelLogMaskingGreenInRed_v11_5	= 0x6d6c6772,
        bmdDeckLinkFrameMetadataCintelLogMaskingBlueInRed_v11_5	= 0x6d6c6272,
        bmdDeckLinkFrameMetadataCintelLogMaskingRedInGreen_v11_5	= 0x6d6c7267,
        bmdDeckLinkFrameMetadataCintelLogMaskingGreenInGreen_v11_5	= 0x6d6c6767,
        bmdDeckLinkFrameMetadataCintelLogMaskingBlueInGreen_v11_5	= 0x6d6c6267,
        bmdDeckLinkFrameMetadataCintelLogMaskingRedInBlue_v11_5	= 0x6d6c7262,
        bmdDeckLinkFrameMetadataCintelLogMaskingGreenInBlue_v11_5	= 0x6d6c6762,
        bmdDeckLinkFrameMetadataCintelLogMaskingBlueInBlue_v11_5	= 0x6d6c6262,
        bmdDeckLinkFrameMetadataCintelFilmFrameRate_v11_5	= 0x63666672,
        bmdDeckLinkFrameMetadataCintelOffsetToApplyHorizontal_v11_5	= 0x6f746168,
        bmdDeckLinkFrameMetadataCintelOffsetToApplyVertical_v11_5	= 0x6f746176,
        bmdDeckLinkFrameMetadataCintelGainRed_v11_5	= 0x4c665264,
        bmdDeckLinkFrameMetadataCintelGainGreen_v11_5	= 0x4c664772,
        bmdDeckLinkFrameMetadataCintelGainBlue_v11_5	= 0x4c66426c,
        bmdDeckLinkFrameMetadataCintelLiftRed_v11_5	= 0x476e5264,
        bmdDeckLinkFrameMetadataCintelLiftGreen_v11_5	= 0x476e4772,
        bmdDeckLinkFrameMetadataCintelLiftBlue_v11_5	= 0x476e426c,
        bmdDeckLinkFrameMetadataCintelHDRGainRed_v11_5	= 0x48475264,
        bmdDeckLinkFrameMetadataCintelHDRGainGreen_v11_5	= 0x48474772,
        bmdDeckLinkFrameMetadataCintelHDRGainBlue_v11_5	= 0x4847426c,
        bmdDeckLinkFrameMetadataCintel16mmCropRequired_v11_5	= 0x63313663,
        bmdDeckLinkFrameMetadataCintelInversionRequired_v11_5	= 0x63696e76,
        bmdDeckLinkFrameMetadataCintelFlipRequired_v11_5	= 0x63666c72,
        bmdDeckLinkFrameMetadataCintelFocusAssistEnabled_v11_5	= 0x63666165,
        bmdDeckLinkFrameMetadataCintelKeykodeIsInterpolated_v11_5	= 0x6b6b6969
    } 	BMDDeckLinkFrameMetadataID_v11_5;


typedef /* [v1_enum] */ 
enum _BMDDeckLinkAttributeID_v10_6
    {
        BMDDeckLinkSupportsDesktopDisplay_v10_6	= 0x65787464
    } 	BMDDeckLinkAttributeID_v10_6;

typedef /* [v1_enum] */ 
enum _BMDIdleVideoOutputOperation_v10_6
    {
        bmdIdleVideoOutputDesktop_v10_6	= 0x6465736b
    } 	BMDIdleVideoOutputOperation_v10_6;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkAttributeID_v10_5
    {
        BMDDeckLinkDeviceBusyState_v10_5	= 0x64627374
    } 	BMDDeckLinkAttributeID_v10_5;



EXTERN_C const IID LIBID_DeckLinkAPI;

#ifndef __IDeckLinkTimecode_INTERFACE_DEFINED__
#define __IDeckLinkTimecode_INTERFACE_DEFINED__

/* interface IDeckLinkTimecode */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkTimecode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BC6CFBD3-8317-4325-AC1C-1216391E9340")
    IDeckLinkTimecode : public IUnknown
    {
    public:
        virtual BMDTimecodeBCD STDMETHODCALLTYPE GetBCD( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetComponents( 
            /* [out] */ unsigned char *hours,
            /* [out] */ unsigned char *minutes,
            /* [out] */ unsigned char *seconds,
            /* [out] */ unsigned char *frames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [out] */ BSTR *timecode) = 0;
        
        virtual BMDTimecodeFlags STDMETHODCALLTYPE GetFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecodeUserBits( 
            /* [out] */ BMDTimecodeUserBits *userBits) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkTimecodeVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkTimecode * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkTimecode * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkTimecode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkTimecode, GetBCD)
        BMDTimecodeBCD ( STDMETHODCALLTYPE *GetBCD )( 
            IDeckLinkTimecode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkTimecode, GetComponents)
        HRESULT ( STDMETHODCALLTYPE *GetComponents )( 
            IDeckLinkTimecode * This,
            /* [out] */ unsigned char *hours,
            /* [out] */ unsigned char *minutes,
            /* [out] */ unsigned char *seconds,
            /* [out] */ unsigned char *frames);
        
        DECLSPEC_XFGVIRT(IDeckLinkTimecode, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkTimecode * This,
            /* [out] */ BSTR *timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkTimecode, GetFlags)
        BMDTimecodeFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkTimecode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkTimecode, GetTimecodeUserBits)
        HRESULT ( STDMETHODCALLTYPE *GetTimecodeUserBits )( 
            IDeckLinkTimecode * This,
            /* [out] */ BMDTimecodeUserBits *userBits);
        
        END_INTERFACE
    } IDeckLinkTimecodeVtbl;

    interface IDeckLinkTimecode
    {
        CONST_VTBL struct IDeckLinkTimecodeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkTimecode_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkTimecode_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkTimecode_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkTimecode_GetBCD(This)	\
    ( (This)->lpVtbl -> GetBCD(This) ) 

#define IDeckLinkTimecode_GetComponents(This,hours,minutes,seconds,frames)	\
    ( (This)->lpVtbl -> GetComponents(This,hours,minutes,seconds,frames) ) 

#define IDeckLinkTimecode_GetString(This,timecode)	\
    ( (This)->lpVtbl -> GetString(This,timecode) ) 

#define IDeckLinkTimecode_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkTimecode_GetTimecodeUserBits(This,userBits)	\
    ( (This)->lpVtbl -> GetTimecodeUserBits(This,userBits) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkTimecode_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDisplayModeIterator_INTERFACE_DEFINED__
#define __IDeckLinkDisplayModeIterator_INTERFACE_DEFINED__

/* interface IDeckLinkDisplayModeIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDisplayModeIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9C88499F-F601-4021-B80B-032E4EB41C35")
    IDeckLinkDisplayModeIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLinkDisplayMode **deckLinkDisplayMode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDisplayModeIteratorVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDisplayModeIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDisplayModeIterator * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDisplayModeIterator * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayModeIterator, Next)
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkDisplayModeIterator * This,
            /* [out] */ IDeckLinkDisplayMode **deckLinkDisplayMode);
        
        END_INTERFACE
    } IDeckLinkDisplayModeIteratorVtbl;

    interface IDeckLinkDisplayModeIterator
    {
        CONST_VTBL struct IDeckLinkDisplayModeIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDisplayModeIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDisplayModeIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDisplayModeIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDisplayModeIterator_Next(This,deckLinkDisplayMode)	\
    ( (This)->lpVtbl -> Next(This,deckLinkDisplayMode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDisplayModeIterator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDisplayMode_INTERFACE_DEFINED__
#define __IDeckLinkDisplayMode_INTERFACE_DEFINED__

/* interface IDeckLinkDisplayMode */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDisplayMode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3EB2C1AB-0A3D-4523-A3AD-F40D7FB14E78")
    IDeckLinkDisplayMode : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *name) = 0;
        
        virtual BMDDisplayMode STDMETHODCALLTYPE GetDisplayMode( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetWidth( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetHeight( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameRate( 
            /* [out] */ BMDTimeValue *frameDuration,
            /* [out] */ BMDTimeScale *timeScale) = 0;
        
        virtual BMDFieldDominance STDMETHODCALLTYPE GetFieldDominance( void) = 0;
        
        virtual BMDDisplayModeFlags STDMETHODCALLTYPE GetFlags( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDisplayModeVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDisplayMode * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDisplayMode * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDisplayMode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayMode, GetName)
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IDeckLinkDisplayMode * This,
            /* [out] */ BSTR *name);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayMode, GetDisplayMode)
        BMDDisplayMode ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkDisplayMode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayMode, GetWidth)
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkDisplayMode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayMode, GetHeight)
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkDisplayMode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayMode, GetFrameRate)
        HRESULT ( STDMETHODCALLTYPE *GetFrameRate )( 
            IDeckLinkDisplayMode * This,
            /* [out] */ BMDTimeValue *frameDuration,
            /* [out] */ BMDTimeScale *timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayMode, GetFieldDominance)
        BMDFieldDominance ( STDMETHODCALLTYPE *GetFieldDominance )( 
            IDeckLinkDisplayMode * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDisplayMode, GetFlags)
        BMDDisplayModeFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkDisplayMode * This);
        
        END_INTERFACE
    } IDeckLinkDisplayModeVtbl;

    interface IDeckLinkDisplayMode
    {
        CONST_VTBL struct IDeckLinkDisplayModeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDisplayMode_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDisplayMode_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDisplayMode_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDisplayMode_GetName(This,name)	\
    ( (This)->lpVtbl -> GetName(This,name) ) 

#define IDeckLinkDisplayMode_GetDisplayMode(This)	\
    ( (This)->lpVtbl -> GetDisplayMode(This) ) 

#define IDeckLinkDisplayMode_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkDisplayMode_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkDisplayMode_GetFrameRate(This,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetFrameRate(This,frameDuration,timeScale) ) 

#define IDeckLinkDisplayMode_GetFieldDominance(This)	\
    ( (This)->lpVtbl -> GetFieldDominance(This) ) 

#define IDeckLinkDisplayMode_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDisplayMode_INTERFACE_DEFINED__ */


#ifndef __IDeckLink_INTERFACE_DEFINED__
#define __IDeckLink_INTERFACE_DEFINED__

/* interface IDeckLink */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C418FBDD-0587-48ED-8FE5-640F0A14AF91")
    IDeckLink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetModelName( 
            /* [out] */ BSTR *modelName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [out] */ BSTR *displayName) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLink * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLink * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLink * This);
        
        DECLSPEC_XFGVIRT(IDeckLink, GetModelName)
        HRESULT ( STDMETHODCALLTYPE *GetModelName )( 
            IDeckLink * This,
            /* [out] */ BSTR *modelName);
        
        DECLSPEC_XFGVIRT(IDeckLink, GetDisplayName)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IDeckLink * This,
            /* [out] */ BSTR *displayName);
        
        END_INTERFACE
    } IDeckLinkVtbl;

    interface IDeckLink
    {
        CONST_VTBL struct IDeckLinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLink_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLink_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLink_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLink_GetModelName(This,modelName)	\
    ( (This)->lpVtbl -> GetModelName(This,modelName) ) 

#define IDeckLink_GetDisplayName(This,displayName)	\
    ( (This)->lpVtbl -> GetDisplayName(This,displayName) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLink_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkConfiguration_INTERFACE_DEFINED__
#define __IDeckLinkConfiguration_INTERFACE_DEFINED__

/* interface IDeckLinkConfiguration */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("912F634B-2D4E-40A4-8AAB-8D80B73F1289")
    IDeckLinkConfiguration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteConfigurationToPreferences( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkConfiguration * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkConfiguration * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkConfiguration * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkConfiguration * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration, WriteConfigurationToPreferences)
        HRESULT ( STDMETHODCALLTYPE *WriteConfigurationToPreferences )( 
            IDeckLinkConfiguration * This);
        
        END_INTERFACE
    } IDeckLinkConfigurationVtbl;

    interface IDeckLinkConfiguration
    {
        CONST_VTBL struct IDeckLinkConfigurationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkConfiguration_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkConfiguration_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkConfiguration_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkConfiguration_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_WriteConfigurationToPreferences(This)	\
    ( (This)->lpVtbl -> WriteConfigurationToPreferences(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkConfiguration_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderConfiguration_INTERFACE_DEFINED__
#define __IDeckLinkEncoderConfiguration_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderConfiguration */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("138050E5-C60A-4552-BF3F-0F358049327E")
    IDeckLinkEncoderConfiguration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ void *buffer,
            /* [out][in] */ unsigned int *bufferSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderConfiguration * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderConfiguration * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkEncoderConfiguration * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ void *buffer,
            /* [out][in] */ unsigned int *bufferSize);
        
        END_INTERFACE
    } IDeckLinkEncoderConfigurationVtbl;

    interface IDeckLinkEncoderConfiguration
    {
        CONST_VTBL struct IDeckLinkEncoderConfigurationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderConfiguration_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderConfiguration_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderConfiguration_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderConfiguration_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_GetBytes(This,cfgID,buffer,bufferSize)	\
    ( (This)->lpVtbl -> GetBytes(This,cfgID,buffer,bufferSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderConfiguration_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDeckControlStatusCallback_INTERFACE_DEFINED__
#define __IDeckLinkDeckControlStatusCallback_INTERFACE_DEFINED__

/* interface IDeckLinkDeckControlStatusCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDeckControlStatusCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("53436FFB-B434-4906-BADC-AE3060FFE8EF")
    IDeckLinkDeckControlStatusCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TimecodeUpdate( 
            /* [in] */ BMDTimecodeBCD currentTimecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VTRControlStateChanged( 
            /* [in] */ BMDDeckControlVTRControlState newState,
            /* [in] */ BMDDeckControlError error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeckControlEventReceived( 
            /* [in] */ BMDDeckControlEvent event,
            /* [in] */ BMDDeckControlError error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeckControlStatusChanged( 
            /* [in] */ BMDDeckControlStatusFlags flags,
            /* [in] */ unsigned int mask) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDeckControlStatusCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDeckControlStatusCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDeckControlStatusCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDeckControlStatusCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControlStatusCallback, TimecodeUpdate)
        HRESULT ( STDMETHODCALLTYPE *TimecodeUpdate )( 
            IDeckLinkDeckControlStatusCallback * This,
            /* [in] */ BMDTimecodeBCD currentTimecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControlStatusCallback, VTRControlStateChanged)
        HRESULT ( STDMETHODCALLTYPE *VTRControlStateChanged )( 
            IDeckLinkDeckControlStatusCallback * This,
            /* [in] */ BMDDeckControlVTRControlState newState,
            /* [in] */ BMDDeckControlError error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControlStatusCallback, DeckControlEventReceived)
        HRESULT ( STDMETHODCALLTYPE *DeckControlEventReceived )( 
            IDeckLinkDeckControlStatusCallback * This,
            /* [in] */ BMDDeckControlEvent event,
            /* [in] */ BMDDeckControlError error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControlStatusCallback, DeckControlStatusChanged)
        HRESULT ( STDMETHODCALLTYPE *DeckControlStatusChanged )( 
            IDeckLinkDeckControlStatusCallback * This,
            /* [in] */ BMDDeckControlStatusFlags flags,
            /* [in] */ unsigned int mask);
        
        END_INTERFACE
    } IDeckLinkDeckControlStatusCallbackVtbl;

    interface IDeckLinkDeckControlStatusCallback
    {
        CONST_VTBL struct IDeckLinkDeckControlStatusCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDeckControlStatusCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDeckControlStatusCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDeckControlStatusCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDeckControlStatusCallback_TimecodeUpdate(This,currentTimecode)	\
    ( (This)->lpVtbl -> TimecodeUpdate(This,currentTimecode) ) 

#define IDeckLinkDeckControlStatusCallback_VTRControlStateChanged(This,newState,error)	\
    ( (This)->lpVtbl -> VTRControlStateChanged(This,newState,error) ) 

#define IDeckLinkDeckControlStatusCallback_DeckControlEventReceived(This,event,error)	\
    ( (This)->lpVtbl -> DeckControlEventReceived(This,event,error) ) 

#define IDeckLinkDeckControlStatusCallback_DeckControlStatusChanged(This,flags,mask)	\
    ( (This)->lpVtbl -> DeckControlStatusChanged(This,flags,mask) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDeckControlStatusCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDeckControl_INTERFACE_DEFINED__
#define __IDeckLinkDeckControl_INTERFACE_DEFINED__

/* interface IDeckLinkDeckControl */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDeckControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8E1C3ACE-19C7-4E00-8B92-D80431D958BE")
    IDeckLinkDeckControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ BMDTimeValue timeValue,
            /* [in] */ BOOL timecodeIsDropFrame,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( 
            /* [in] */ BOOL standbyOn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentState( 
            /* [out] */ BMDDeckControlMode *mode,
            /* [out] */ BMDDeckControlVTRControlState *vtrControlState,
            /* [out] */ BMDDeckControlStatusFlags *flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStandby( 
            /* [in] */ BOOL standbyOn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendCommand( 
            /* [in] */ unsigned char *inBuffer,
            /* [in] */ unsigned int inBufferSize,
            /* [out] */ unsigned char *outBuffer,
            /* [out] */ unsigned int *outDataSize,
            /* [in] */ unsigned int outBufferSize,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Play( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TogglePlayStop( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Eject( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GoToTimecode( 
            /* [in] */ BMDTimecodeBCD timecode,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FastForward( 
            /* [in] */ BOOL viewTape,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Rewind( 
            /* [in] */ BOOL viewTape,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepForward( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepBack( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Jog( 
            /* [in] */ double rate,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Shuttle( 
            /* [in] */ double rate,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecodeString( 
            /* [out] */ BSTR *currentTimeCode,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecode( 
            /* [out] */ IDeckLinkTimecode **currentTimecode,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecodeBCD( 
            /* [out] */ BMDTimecodeBCD *currentTimecode,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPreroll( 
            /* [in] */ unsigned int prerollSeconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreroll( 
            /* [out] */ unsigned int *prerollSeconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExportOffset( 
            /* [in] */ int exportOffsetFields) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExportOffset( 
            /* [out] */ int *exportOffsetFields) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManualExportOffset( 
            /* [out] */ int *deckManualExportOffsetFields) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCaptureOffset( 
            /* [in] */ int captureOffsetFields) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCaptureOffset( 
            /* [out] */ int *captureOffsetFields) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartExport( 
            /* [in] */ BMDTimecodeBCD inTimecode,
            /* [in] */ BMDTimecodeBCD outTimecode,
            /* [in] */ BMDDeckControlExportModeOpsFlags exportModeOps,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartCapture( 
            /* [in] */ BOOL useVITC,
            /* [in] */ BMDTimecodeBCD inTimecode,
            /* [in] */ BMDTimecodeBCD outTimecode,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceID( 
            /* [out] */ unsigned short *deviceId,
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CrashRecordStart( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CrashRecordStop( 
            /* [out] */ BMDDeckControlError *error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkDeckControlStatusCallback *callback) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDeckControlVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDeckControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDeckControl * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDeckControl * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Open)
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ BMDTimeValue timeValue,
            /* [in] */ BOOL timecodeIsDropFrame,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Close)
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BOOL standbyOn);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetCurrentState)
        HRESULT ( STDMETHODCALLTYPE *GetCurrentState )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlMode *mode,
            /* [out] */ BMDDeckControlVTRControlState *vtrControlState,
            /* [out] */ BMDDeckControlStatusFlags *flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, SetStandby)
        HRESULT ( STDMETHODCALLTYPE *SetStandby )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BOOL standbyOn);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, SendCommand)
        HRESULT ( STDMETHODCALLTYPE *SendCommand )( 
            IDeckLinkDeckControl * This,
            /* [in] */ unsigned char *inBuffer,
            /* [in] */ unsigned int inBufferSize,
            /* [out] */ unsigned char *outBuffer,
            /* [out] */ unsigned int *outDataSize,
            /* [in] */ unsigned int outBufferSize,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Play)
        HRESULT ( STDMETHODCALLTYPE *Play )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Stop)
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, TogglePlayStop)
        HRESULT ( STDMETHODCALLTYPE *TogglePlayStop )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Eject)
        HRESULT ( STDMETHODCALLTYPE *Eject )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GoToTimecode)
        HRESULT ( STDMETHODCALLTYPE *GoToTimecode )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BMDTimecodeBCD timecode,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, FastForward)
        HRESULT ( STDMETHODCALLTYPE *FastForward )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BOOL viewTape,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Rewind)
        HRESULT ( STDMETHODCALLTYPE *Rewind )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BOOL viewTape,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, StepForward)
        HRESULT ( STDMETHODCALLTYPE *StepForward )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, StepBack)
        HRESULT ( STDMETHODCALLTYPE *StepBack )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Jog)
        HRESULT ( STDMETHODCALLTYPE *Jog )( 
            IDeckLinkDeckControl * This,
            /* [in] */ double rate,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Shuttle)
        HRESULT ( STDMETHODCALLTYPE *Shuttle )( 
            IDeckLinkDeckControl * This,
            /* [in] */ double rate,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetTimecodeString)
        HRESULT ( STDMETHODCALLTYPE *GetTimecodeString )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BSTR *currentTimeCode,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkDeckControl * This,
            /* [out] */ IDeckLinkTimecode **currentTimecode,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetTimecodeBCD)
        HRESULT ( STDMETHODCALLTYPE *GetTimecodeBCD )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDTimecodeBCD *currentTimecode,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, SetPreroll)
        HRESULT ( STDMETHODCALLTYPE *SetPreroll )( 
            IDeckLinkDeckControl * This,
            /* [in] */ unsigned int prerollSeconds);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetPreroll)
        HRESULT ( STDMETHODCALLTYPE *GetPreroll )( 
            IDeckLinkDeckControl * This,
            /* [out] */ unsigned int *prerollSeconds);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, SetExportOffset)
        HRESULT ( STDMETHODCALLTYPE *SetExportOffset )( 
            IDeckLinkDeckControl * This,
            /* [in] */ int exportOffsetFields);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetExportOffset)
        HRESULT ( STDMETHODCALLTYPE *GetExportOffset )( 
            IDeckLinkDeckControl * This,
            /* [out] */ int *exportOffsetFields);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetManualExportOffset)
        HRESULT ( STDMETHODCALLTYPE *GetManualExportOffset )( 
            IDeckLinkDeckControl * This,
            /* [out] */ int *deckManualExportOffsetFields);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, SetCaptureOffset)
        HRESULT ( STDMETHODCALLTYPE *SetCaptureOffset )( 
            IDeckLinkDeckControl * This,
            /* [in] */ int captureOffsetFields);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetCaptureOffset)
        HRESULT ( STDMETHODCALLTYPE *GetCaptureOffset )( 
            IDeckLinkDeckControl * This,
            /* [out] */ int *captureOffsetFields);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, StartExport)
        HRESULT ( STDMETHODCALLTYPE *StartExport )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BMDTimecodeBCD inTimecode,
            /* [in] */ BMDTimecodeBCD outTimecode,
            /* [in] */ BMDDeckControlExportModeOpsFlags exportModeOps,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, StartCapture)
        HRESULT ( STDMETHODCALLTYPE *StartCapture )( 
            IDeckLinkDeckControl * This,
            /* [in] */ BOOL useVITC,
            /* [in] */ BMDTimecodeBCD inTimecode,
            /* [in] */ BMDTimecodeBCD outTimecode,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, GetDeviceID)
        HRESULT ( STDMETHODCALLTYPE *GetDeviceID )( 
            IDeckLinkDeckControl * This,
            /* [out] */ unsigned short *deviceId,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, Abort)
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IDeckLinkDeckControl * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, CrashRecordStart)
        HRESULT ( STDMETHODCALLTYPE *CrashRecordStart )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, CrashRecordStop)
        HRESULT ( STDMETHODCALLTYPE *CrashRecordStop )( 
            IDeckLinkDeckControl * This,
            /* [out] */ BMDDeckControlError *error);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeckControl, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkDeckControl * This,
            /* [in] */ IDeckLinkDeckControlStatusCallback *callback);
        
        END_INTERFACE
    } IDeckLinkDeckControlVtbl;

    interface IDeckLinkDeckControl
    {
        CONST_VTBL struct IDeckLinkDeckControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDeckControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDeckControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDeckControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDeckControl_Open(This,timeScale,timeValue,timecodeIsDropFrame,error)	\
    ( (This)->lpVtbl -> Open(This,timeScale,timeValue,timecodeIsDropFrame,error) ) 

#define IDeckLinkDeckControl_Close(This,standbyOn)	\
    ( (This)->lpVtbl -> Close(This,standbyOn) ) 

#define IDeckLinkDeckControl_GetCurrentState(This,mode,vtrControlState,flags)	\
    ( (This)->lpVtbl -> GetCurrentState(This,mode,vtrControlState,flags) ) 

#define IDeckLinkDeckControl_SetStandby(This,standbyOn)	\
    ( (This)->lpVtbl -> SetStandby(This,standbyOn) ) 

#define IDeckLinkDeckControl_SendCommand(This,inBuffer,inBufferSize,outBuffer,outDataSize,outBufferSize,error)	\
    ( (This)->lpVtbl -> SendCommand(This,inBuffer,inBufferSize,outBuffer,outDataSize,outBufferSize,error) ) 

#define IDeckLinkDeckControl_Play(This,error)	\
    ( (This)->lpVtbl -> Play(This,error) ) 

#define IDeckLinkDeckControl_Stop(This,error)	\
    ( (This)->lpVtbl -> Stop(This,error) ) 

#define IDeckLinkDeckControl_TogglePlayStop(This,error)	\
    ( (This)->lpVtbl -> TogglePlayStop(This,error) ) 

#define IDeckLinkDeckControl_Eject(This,error)	\
    ( (This)->lpVtbl -> Eject(This,error) ) 

#define IDeckLinkDeckControl_GoToTimecode(This,timecode,error)	\
    ( (This)->lpVtbl -> GoToTimecode(This,timecode,error) ) 

#define IDeckLinkDeckControl_FastForward(This,viewTape,error)	\
    ( (This)->lpVtbl -> FastForward(This,viewTape,error) ) 

#define IDeckLinkDeckControl_Rewind(This,viewTape,error)	\
    ( (This)->lpVtbl -> Rewind(This,viewTape,error) ) 

#define IDeckLinkDeckControl_StepForward(This,error)	\
    ( (This)->lpVtbl -> StepForward(This,error) ) 

#define IDeckLinkDeckControl_StepBack(This,error)	\
    ( (This)->lpVtbl -> StepBack(This,error) ) 

#define IDeckLinkDeckControl_Jog(This,rate,error)	\
    ( (This)->lpVtbl -> Jog(This,rate,error) ) 

#define IDeckLinkDeckControl_Shuttle(This,rate,error)	\
    ( (This)->lpVtbl -> Shuttle(This,rate,error) ) 

#define IDeckLinkDeckControl_GetTimecodeString(This,currentTimeCode,error)	\
    ( (This)->lpVtbl -> GetTimecodeString(This,currentTimeCode,error) ) 

#define IDeckLinkDeckControl_GetTimecode(This,currentTimecode,error)	\
    ( (This)->lpVtbl -> GetTimecode(This,currentTimecode,error) ) 

#define IDeckLinkDeckControl_GetTimecodeBCD(This,currentTimecode,error)	\
    ( (This)->lpVtbl -> GetTimecodeBCD(This,currentTimecode,error) ) 

#define IDeckLinkDeckControl_SetPreroll(This,prerollSeconds)	\
    ( (This)->lpVtbl -> SetPreroll(This,prerollSeconds) ) 

#define IDeckLinkDeckControl_GetPreroll(This,prerollSeconds)	\
    ( (This)->lpVtbl -> GetPreroll(This,prerollSeconds) ) 

#define IDeckLinkDeckControl_SetExportOffset(This,exportOffsetFields)	\
    ( (This)->lpVtbl -> SetExportOffset(This,exportOffsetFields) ) 

#define IDeckLinkDeckControl_GetExportOffset(This,exportOffsetFields)	\
    ( (This)->lpVtbl -> GetExportOffset(This,exportOffsetFields) ) 

#define IDeckLinkDeckControl_GetManualExportOffset(This,deckManualExportOffsetFields)	\
    ( (This)->lpVtbl -> GetManualExportOffset(This,deckManualExportOffsetFields) ) 

#define IDeckLinkDeckControl_SetCaptureOffset(This,captureOffsetFields)	\
    ( (This)->lpVtbl -> SetCaptureOffset(This,captureOffsetFields) ) 

#define IDeckLinkDeckControl_GetCaptureOffset(This,captureOffsetFields)	\
    ( (This)->lpVtbl -> GetCaptureOffset(This,captureOffsetFields) ) 

#define IDeckLinkDeckControl_StartExport(This,inTimecode,outTimecode,exportModeOps,error)	\
    ( (This)->lpVtbl -> StartExport(This,inTimecode,outTimecode,exportModeOps,error) ) 

#define IDeckLinkDeckControl_StartCapture(This,useVITC,inTimecode,outTimecode,error)	\
    ( (This)->lpVtbl -> StartCapture(This,useVITC,inTimecode,outTimecode,error) ) 

#define IDeckLinkDeckControl_GetDeviceID(This,deviceId,error)	\
    ( (This)->lpVtbl -> GetDeviceID(This,deviceId,error) ) 

#define IDeckLinkDeckControl_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define IDeckLinkDeckControl_CrashRecordStart(This,error)	\
    ( (This)->lpVtbl -> CrashRecordStart(This,error) ) 

#define IDeckLinkDeckControl_CrashRecordStop(This,error)	\
    ( (This)->lpVtbl -> CrashRecordStop(This,error) ) 

#define IDeckLinkDeckControl_SetCallback(This,callback)	\
    ( (This)->lpVtbl -> SetCallback(This,callback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDeckControl_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingDeviceNotificationCallback_INTERFACE_DEFINED__
#define __IBMDStreamingDeviceNotificationCallback_INTERFACE_DEFINED__

/* interface IBMDStreamingDeviceNotificationCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingDeviceNotificationCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F9531D64-3305-4B29-A387-7F74BB0D0E84")
    IBMDStreamingDeviceNotificationCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StreamingDeviceArrived( 
            /* [in] */ IDeckLink *device) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StreamingDeviceRemoved( 
            /* [in] */ IDeckLink *device) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StreamingDeviceModeChanged( 
            /* [in] */ IDeckLink *device,
            /* [in] */ BMDStreamingDeviceMode mode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingDeviceNotificationCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingDeviceNotificationCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingDeviceNotificationCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingDeviceNotificationCallback * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceNotificationCallback, StreamingDeviceArrived)
        HRESULT ( STDMETHODCALLTYPE *StreamingDeviceArrived )( 
            IBMDStreamingDeviceNotificationCallback * This,
            /* [in] */ IDeckLink *device);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceNotificationCallback, StreamingDeviceRemoved)
        HRESULT ( STDMETHODCALLTYPE *StreamingDeviceRemoved )( 
            IBMDStreamingDeviceNotificationCallback * This,
            /* [in] */ IDeckLink *device);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceNotificationCallback, StreamingDeviceModeChanged)
        HRESULT ( STDMETHODCALLTYPE *StreamingDeviceModeChanged )( 
            IBMDStreamingDeviceNotificationCallback * This,
            /* [in] */ IDeckLink *device,
            /* [in] */ BMDStreamingDeviceMode mode);
        
        END_INTERFACE
    } IBMDStreamingDeviceNotificationCallbackVtbl;

    interface IBMDStreamingDeviceNotificationCallback
    {
        CONST_VTBL struct IBMDStreamingDeviceNotificationCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingDeviceNotificationCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingDeviceNotificationCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingDeviceNotificationCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingDeviceNotificationCallback_StreamingDeviceArrived(This,device)	\
    ( (This)->lpVtbl -> StreamingDeviceArrived(This,device) ) 

#define IBMDStreamingDeviceNotificationCallback_StreamingDeviceRemoved(This,device)	\
    ( (This)->lpVtbl -> StreamingDeviceRemoved(This,device) ) 

#define IBMDStreamingDeviceNotificationCallback_StreamingDeviceModeChanged(This,device,mode)	\
    ( (This)->lpVtbl -> StreamingDeviceModeChanged(This,device,mode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingDeviceNotificationCallback_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingH264InputCallback_INTERFACE_DEFINED__
#define __IBMDStreamingH264InputCallback_INTERFACE_DEFINED__

/* interface IBMDStreamingH264InputCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingH264InputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("823C475F-55AE-46F9-890C-537CC5CEDCCA")
    IBMDStreamingH264InputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE H264NALPacketArrived( 
            /* [in] */ IBMDStreamingH264NALPacket *nalPacket) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE H264AudioPacketArrived( 
            /* [in] */ IBMDStreamingAudioPacket *audioPacket) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MPEG2TSPacketArrived( 
            /* [in] */ IBMDStreamingMPEG2TSPacket *tsPacket) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE H264VideoInputConnectorScanningChanged( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE H264VideoInputConnectorChanged( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE H264VideoInputModeChanged( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingH264InputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingH264InputCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingH264InputCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingH264InputCallback * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264InputCallback, H264NALPacketArrived)
        HRESULT ( STDMETHODCALLTYPE *H264NALPacketArrived )( 
            IBMDStreamingH264InputCallback * This,
            /* [in] */ IBMDStreamingH264NALPacket *nalPacket);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264InputCallback, H264AudioPacketArrived)
        HRESULT ( STDMETHODCALLTYPE *H264AudioPacketArrived )( 
            IBMDStreamingH264InputCallback * This,
            /* [in] */ IBMDStreamingAudioPacket *audioPacket);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264InputCallback, MPEG2TSPacketArrived)
        HRESULT ( STDMETHODCALLTYPE *MPEG2TSPacketArrived )( 
            IBMDStreamingH264InputCallback * This,
            /* [in] */ IBMDStreamingMPEG2TSPacket *tsPacket);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264InputCallback, H264VideoInputConnectorScanningChanged)
        HRESULT ( STDMETHODCALLTYPE *H264VideoInputConnectorScanningChanged )( 
            IBMDStreamingH264InputCallback * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264InputCallback, H264VideoInputConnectorChanged)
        HRESULT ( STDMETHODCALLTYPE *H264VideoInputConnectorChanged )( 
            IBMDStreamingH264InputCallback * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264InputCallback, H264VideoInputModeChanged)
        HRESULT ( STDMETHODCALLTYPE *H264VideoInputModeChanged )( 
            IBMDStreamingH264InputCallback * This);
        
        END_INTERFACE
    } IBMDStreamingH264InputCallbackVtbl;

    interface IBMDStreamingH264InputCallback
    {
        CONST_VTBL struct IBMDStreamingH264InputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingH264InputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingH264InputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingH264InputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingH264InputCallback_H264NALPacketArrived(This,nalPacket)	\
    ( (This)->lpVtbl -> H264NALPacketArrived(This,nalPacket) ) 

#define IBMDStreamingH264InputCallback_H264AudioPacketArrived(This,audioPacket)	\
    ( (This)->lpVtbl -> H264AudioPacketArrived(This,audioPacket) ) 

#define IBMDStreamingH264InputCallback_MPEG2TSPacketArrived(This,tsPacket)	\
    ( (This)->lpVtbl -> MPEG2TSPacketArrived(This,tsPacket) ) 

#define IBMDStreamingH264InputCallback_H264VideoInputConnectorScanningChanged(This)	\
    ( (This)->lpVtbl -> H264VideoInputConnectorScanningChanged(This) ) 

#define IBMDStreamingH264InputCallback_H264VideoInputConnectorChanged(This)	\
    ( (This)->lpVtbl -> H264VideoInputConnectorChanged(This) ) 

#define IBMDStreamingH264InputCallback_H264VideoInputModeChanged(This)	\
    ( (This)->lpVtbl -> H264VideoInputModeChanged(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingH264InputCallback_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingDiscovery_INTERFACE_DEFINED__
#define __IBMDStreamingDiscovery_INTERFACE_DEFINED__

/* interface IBMDStreamingDiscovery */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingDiscovery;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2C837444-F989-4D87-901A-47C8A36D096D")
    IBMDStreamingDiscovery : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InstallDeviceNotifications( 
            /* [in] */ IBMDStreamingDeviceNotificationCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UninstallDeviceNotifications( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingDiscoveryVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingDiscovery * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingDiscovery * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingDiscovery * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDiscovery, InstallDeviceNotifications)
        HRESULT ( STDMETHODCALLTYPE *InstallDeviceNotifications )( 
            IBMDStreamingDiscovery * This,
            /* [in] */ IBMDStreamingDeviceNotificationCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDiscovery, UninstallDeviceNotifications)
        HRESULT ( STDMETHODCALLTYPE *UninstallDeviceNotifications )( 
            IBMDStreamingDiscovery * This);
        
        END_INTERFACE
    } IBMDStreamingDiscoveryVtbl;

    interface IBMDStreamingDiscovery
    {
        CONST_VTBL struct IBMDStreamingDiscoveryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingDiscovery_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingDiscovery_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingDiscovery_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingDiscovery_InstallDeviceNotifications(This,theCallback)	\
    ( (This)->lpVtbl -> InstallDeviceNotifications(This,theCallback) ) 

#define IBMDStreamingDiscovery_UninstallDeviceNotifications(This)	\
    ( (This)->lpVtbl -> UninstallDeviceNotifications(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingDiscovery_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingVideoEncodingMode_INTERFACE_DEFINED__
#define __IBMDStreamingVideoEncodingMode_INTERFACE_DEFINED__

/* interface IBMDStreamingVideoEncodingMode */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingVideoEncodingMode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1AB8035B-CD13-458D-B6DF-5E8F7C2141D9")
    IBMDStreamingVideoEncodingMode : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *name) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetPresetID( void) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetSourcePositionX( void) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetSourcePositionY( void) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetSourceWidth( void) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetSourceHeight( void) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetDestWidth( void) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetDestHeight( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateMutableVideoEncodingMode( 
            /* [out] */ IBMDStreamingMutableVideoEncodingMode **newEncodingMode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingVideoEncodingModeVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingVideoEncodingMode * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetName)
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IBMDStreamingVideoEncodingMode * This,
            /* [out] */ BSTR *name);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetPresetID)
        unsigned int ( STDMETHODCALLTYPE *GetPresetID )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourcePositionX)
        unsigned int ( STDMETHODCALLTYPE *GetSourcePositionX )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourcePositionY)
        unsigned int ( STDMETHODCALLTYPE *GetSourcePositionY )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourceWidth)
        unsigned int ( STDMETHODCALLTYPE *GetSourceWidth )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourceHeight)
        unsigned int ( STDMETHODCALLTYPE *GetSourceHeight )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetDestWidth)
        unsigned int ( STDMETHODCALLTYPE *GetDestWidth )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetDestHeight)
        unsigned int ( STDMETHODCALLTYPE *GetDestHeight )( 
            IBMDStreamingVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IBMDStreamingVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IBMDStreamingVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IBMDStreamingVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IBMDStreamingVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, CreateMutableVideoEncodingMode)
        HRESULT ( STDMETHODCALLTYPE *CreateMutableVideoEncodingMode )( 
            IBMDStreamingVideoEncodingMode * This,
            /* [out] */ IBMDStreamingMutableVideoEncodingMode **newEncodingMode);
        
        END_INTERFACE
    } IBMDStreamingVideoEncodingModeVtbl;

    interface IBMDStreamingVideoEncodingMode
    {
        CONST_VTBL struct IBMDStreamingVideoEncodingModeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingVideoEncodingMode_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingVideoEncodingMode_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingVideoEncodingMode_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingVideoEncodingMode_GetName(This,name)	\
    ( (This)->lpVtbl -> GetName(This,name) ) 

#define IBMDStreamingVideoEncodingMode_GetPresetID(This)	\
    ( (This)->lpVtbl -> GetPresetID(This) ) 

#define IBMDStreamingVideoEncodingMode_GetSourcePositionX(This)	\
    ( (This)->lpVtbl -> GetSourcePositionX(This) ) 

#define IBMDStreamingVideoEncodingMode_GetSourcePositionY(This)	\
    ( (This)->lpVtbl -> GetSourcePositionY(This) ) 

#define IBMDStreamingVideoEncodingMode_GetSourceWidth(This)	\
    ( (This)->lpVtbl -> GetSourceWidth(This) ) 

#define IBMDStreamingVideoEncodingMode_GetSourceHeight(This)	\
    ( (This)->lpVtbl -> GetSourceHeight(This) ) 

#define IBMDStreamingVideoEncodingMode_GetDestWidth(This)	\
    ( (This)->lpVtbl -> GetDestWidth(This) ) 

#define IBMDStreamingVideoEncodingMode_GetDestHeight(This)	\
    ( (This)->lpVtbl -> GetDestHeight(This) ) 

#define IBMDStreamingVideoEncodingMode_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IBMDStreamingVideoEncodingMode_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IBMDStreamingVideoEncodingMode_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IBMDStreamingVideoEncodingMode_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IBMDStreamingVideoEncodingMode_CreateMutableVideoEncodingMode(This,newEncodingMode)	\
    ( (This)->lpVtbl -> CreateMutableVideoEncodingMode(This,newEncodingMode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingVideoEncodingMode_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingMutableVideoEncodingMode_INTERFACE_DEFINED__
#define __IBMDStreamingMutableVideoEncodingMode_INTERFACE_DEFINED__

/* interface IBMDStreamingMutableVideoEncodingMode */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingMutableVideoEncodingMode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("19BF7D90-1E0A-400D-B2C6-FFC4E78AD49D")
    IBMDStreamingMutableVideoEncodingMode : public IBMDStreamingVideoEncodingMode
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSourceRect( 
            /* [in] */ unsigned int posX,
            /* [in] */ unsigned int posY,
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDestSize( 
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ BSTR value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingMutableVideoEncodingModeVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetName)
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [out] */ BSTR *name);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetPresetID)
        unsigned int ( STDMETHODCALLTYPE *GetPresetID )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourcePositionX)
        unsigned int ( STDMETHODCALLTYPE *GetSourcePositionX )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourcePositionY)
        unsigned int ( STDMETHODCALLTYPE *GetSourcePositionY )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourceWidth)
        unsigned int ( STDMETHODCALLTYPE *GetSourceWidth )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetSourceHeight)
        unsigned int ( STDMETHODCALLTYPE *GetSourceHeight )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetDestWidth)
        unsigned int ( STDMETHODCALLTYPE *GetDestWidth )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetDestHeight)
        unsigned int ( STDMETHODCALLTYPE *GetDestHeight )( 
            IBMDStreamingMutableVideoEncodingMode * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingMode, CreateMutableVideoEncodingMode)
        HRESULT ( STDMETHODCALLTYPE *CreateMutableVideoEncodingMode )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [out] */ IBMDStreamingMutableVideoEncodingMode **newEncodingMode);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMutableVideoEncodingMode, SetSourceRect)
        HRESULT ( STDMETHODCALLTYPE *SetSourceRect )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ unsigned int posX,
            /* [in] */ unsigned int posY,
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMutableVideoEncodingMode, SetDestSize)
        HRESULT ( STDMETHODCALLTYPE *SetDestSize )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMutableVideoEncodingMode, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMutableVideoEncodingMode, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMutableVideoEncodingMode, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMutableVideoEncodingMode, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IBMDStreamingMutableVideoEncodingMode * This,
            /* [in] */ BMDStreamingEncodingModePropertyID cfgID,
            /* [in] */ BSTR value);
        
        END_INTERFACE
    } IBMDStreamingMutableVideoEncodingModeVtbl;

    interface IBMDStreamingMutableVideoEncodingMode
    {
        CONST_VTBL struct IBMDStreamingMutableVideoEncodingModeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingMutableVideoEncodingMode_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingMutableVideoEncodingMode_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingMutableVideoEncodingMode_GetName(This,name)	\
    ( (This)->lpVtbl -> GetName(This,name) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetPresetID(This)	\
    ( (This)->lpVtbl -> GetPresetID(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetSourcePositionX(This)	\
    ( (This)->lpVtbl -> GetSourcePositionX(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetSourcePositionY(This)	\
    ( (This)->lpVtbl -> GetSourcePositionY(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetSourceWidth(This)	\
    ( (This)->lpVtbl -> GetSourceWidth(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetSourceHeight(This)	\
    ( (This)->lpVtbl -> GetSourceHeight(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetDestWidth(This)	\
    ( (This)->lpVtbl -> GetDestWidth(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetDestHeight(This)	\
    ( (This)->lpVtbl -> GetDestHeight(This) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IBMDStreamingMutableVideoEncodingMode_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IBMDStreamingMutableVideoEncodingMode_CreateMutableVideoEncodingMode(This,newEncodingMode)	\
    ( (This)->lpVtbl -> CreateMutableVideoEncodingMode(This,newEncodingMode) ) 


#define IBMDStreamingMutableVideoEncodingMode_SetSourceRect(This,posX,posY,width,height)	\
    ( (This)->lpVtbl -> SetSourceRect(This,posX,posY,width,height) ) 

#define IBMDStreamingMutableVideoEncodingMode_SetDestSize(This,width,height)	\
    ( (This)->lpVtbl -> SetDestSize(This,width,height) ) 

#define IBMDStreamingMutableVideoEncodingMode_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IBMDStreamingMutableVideoEncodingMode_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IBMDStreamingMutableVideoEncodingMode_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IBMDStreamingMutableVideoEncodingMode_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingMutableVideoEncodingMode_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingVideoEncodingModePresetIterator_INTERFACE_DEFINED__
#define __IBMDStreamingVideoEncodingModePresetIterator_INTERFACE_DEFINED__

/* interface IBMDStreamingVideoEncodingModePresetIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingVideoEncodingModePresetIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7AC731A3-C950-4AD0-804A-8377AA51C6C4")
    IBMDStreamingVideoEncodingModePresetIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IBMDStreamingVideoEncodingMode **videoEncodingMode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingVideoEncodingModePresetIteratorVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingVideoEncodingModePresetIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingVideoEncodingModePresetIterator * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingVideoEncodingModePresetIterator * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingVideoEncodingModePresetIterator, Next)
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IBMDStreamingVideoEncodingModePresetIterator * This,
            /* [out] */ IBMDStreamingVideoEncodingMode **videoEncodingMode);
        
        END_INTERFACE
    } IBMDStreamingVideoEncodingModePresetIteratorVtbl;

    interface IBMDStreamingVideoEncodingModePresetIterator
    {
        CONST_VTBL struct IBMDStreamingVideoEncodingModePresetIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingVideoEncodingModePresetIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingVideoEncodingModePresetIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingVideoEncodingModePresetIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingVideoEncodingModePresetIterator_Next(This,videoEncodingMode)	\
    ( (This)->lpVtbl -> Next(This,videoEncodingMode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingVideoEncodingModePresetIterator_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingDeviceInput_INTERFACE_DEFINED__
#define __IBMDStreamingDeviceInput_INTERFACE_DEFINED__

/* interface IBMDStreamingDeviceInput */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingDeviceInput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("24B6B6EC-1727-44BB-9818-34FF086ACF98")
    IBMDStreamingDeviceInput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoInputMode( 
            /* [in] */ BMDDisplayMode inputMode,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoInputModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoInputMode( 
            /* [in] */ BMDDisplayMode inputMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentDetectedVideoInputMode( 
            /* [out] */ BMDDisplayMode *detectedMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoEncodingMode( 
            /* [out] */ IBMDStreamingVideoEncodingMode **encodingMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoEncodingModePresetIterator( 
            /* [in] */ BMDDisplayMode inputMode,
            /* [out] */ IBMDStreamingVideoEncodingModePresetIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoEncodingMode( 
            /* [in] */ BMDDisplayMode inputMode,
            /* [in] */ IBMDStreamingVideoEncodingMode *encodingMode,
            /* [out] */ BMDStreamingEncodingSupport *result,
            /* [out] */ IBMDStreamingVideoEncodingMode **changedEncodingMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoEncodingMode( 
            /* [in] */ IBMDStreamingVideoEncodingMode *encodingMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartCapture( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopCapture( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IUnknown *theCallback) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingDeviceInputVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingDeviceInput * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingDeviceInput * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingDeviceInput * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, DoesSupportVideoInputMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoInputMode )( 
            IBMDStreamingDeviceInput * This,
            /* [in] */ BMDDisplayMode inputMode,
            /* [out] */ BOOL *result);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, GetVideoInputModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetVideoInputModeIterator )( 
            IBMDStreamingDeviceInput * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, SetVideoInputMode)
        HRESULT ( STDMETHODCALLTYPE *SetVideoInputMode )( 
            IBMDStreamingDeviceInput * This,
            /* [in] */ BMDDisplayMode inputMode);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, GetCurrentDetectedVideoInputMode)
        HRESULT ( STDMETHODCALLTYPE *GetCurrentDetectedVideoInputMode )( 
            IBMDStreamingDeviceInput * This,
            /* [out] */ BMDDisplayMode *detectedMode);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, GetVideoEncodingMode)
        HRESULT ( STDMETHODCALLTYPE *GetVideoEncodingMode )( 
            IBMDStreamingDeviceInput * This,
            /* [out] */ IBMDStreamingVideoEncodingMode **encodingMode);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, GetVideoEncodingModePresetIterator)
        HRESULT ( STDMETHODCALLTYPE *GetVideoEncodingModePresetIterator )( 
            IBMDStreamingDeviceInput * This,
            /* [in] */ BMDDisplayMode inputMode,
            /* [out] */ IBMDStreamingVideoEncodingModePresetIterator **iterator);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, DoesSupportVideoEncodingMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoEncodingMode )( 
            IBMDStreamingDeviceInput * This,
            /* [in] */ BMDDisplayMode inputMode,
            /* [in] */ IBMDStreamingVideoEncodingMode *encodingMode,
            /* [out] */ BMDStreamingEncodingSupport *result,
            /* [out] */ IBMDStreamingVideoEncodingMode **changedEncodingMode);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, SetVideoEncodingMode)
        HRESULT ( STDMETHODCALLTYPE *SetVideoEncodingMode )( 
            IBMDStreamingDeviceInput * This,
            /* [in] */ IBMDStreamingVideoEncodingMode *encodingMode);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, StartCapture)
        HRESULT ( STDMETHODCALLTYPE *StartCapture )( 
            IBMDStreamingDeviceInput * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, StopCapture)
        HRESULT ( STDMETHODCALLTYPE *StopCapture )( 
            IBMDStreamingDeviceInput * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingDeviceInput, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IBMDStreamingDeviceInput * This,
            /* [in] */ IUnknown *theCallback);
        
        END_INTERFACE
    } IBMDStreamingDeviceInputVtbl;

    interface IBMDStreamingDeviceInput
    {
        CONST_VTBL struct IBMDStreamingDeviceInputVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingDeviceInput_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingDeviceInput_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingDeviceInput_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingDeviceInput_DoesSupportVideoInputMode(This,inputMode,result)	\
    ( (This)->lpVtbl -> DoesSupportVideoInputMode(This,inputMode,result) ) 

#define IBMDStreamingDeviceInput_GetVideoInputModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetVideoInputModeIterator(This,iterator) ) 

#define IBMDStreamingDeviceInput_SetVideoInputMode(This,inputMode)	\
    ( (This)->lpVtbl -> SetVideoInputMode(This,inputMode) ) 

#define IBMDStreamingDeviceInput_GetCurrentDetectedVideoInputMode(This,detectedMode)	\
    ( (This)->lpVtbl -> GetCurrentDetectedVideoInputMode(This,detectedMode) ) 

#define IBMDStreamingDeviceInput_GetVideoEncodingMode(This,encodingMode)	\
    ( (This)->lpVtbl -> GetVideoEncodingMode(This,encodingMode) ) 

#define IBMDStreamingDeviceInput_GetVideoEncodingModePresetIterator(This,inputMode,iterator)	\
    ( (This)->lpVtbl -> GetVideoEncodingModePresetIterator(This,inputMode,iterator) ) 

#define IBMDStreamingDeviceInput_DoesSupportVideoEncodingMode(This,inputMode,encodingMode,result,changedEncodingMode)	\
    ( (This)->lpVtbl -> DoesSupportVideoEncodingMode(This,inputMode,encodingMode,result,changedEncodingMode) ) 

#define IBMDStreamingDeviceInput_SetVideoEncodingMode(This,encodingMode)	\
    ( (This)->lpVtbl -> SetVideoEncodingMode(This,encodingMode) ) 

#define IBMDStreamingDeviceInput_StartCapture(This)	\
    ( (This)->lpVtbl -> StartCapture(This) ) 

#define IBMDStreamingDeviceInput_StopCapture(This)	\
    ( (This)->lpVtbl -> StopCapture(This) ) 

#define IBMDStreamingDeviceInput_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingDeviceInput_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingH264NALPacket_INTERFACE_DEFINED__
#define __IBMDStreamingH264NALPacket_INTERFACE_DEFINED__

/* interface IBMDStreamingH264NALPacket */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingH264NALPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E260E955-14BE-4395-9775-9F02CC0A9D89")
    IBMDStreamingH264NALPacket : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetPayloadSize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytesWithSizePrefix( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayTime( 
            /* [in] */ ULONGLONG requestedTimeScale,
            /* [out] */ ULONGLONG *displayTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPacketIndex( 
            /* [out] */ unsigned int *packetIndex) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingH264NALPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingH264NALPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingH264NALPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingH264NALPacket * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALPacket, GetPayloadSize)
        long ( STDMETHODCALLTYPE *GetPayloadSize )( 
            IBMDStreamingH264NALPacket * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IBMDStreamingH264NALPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALPacket, GetBytesWithSizePrefix)
        HRESULT ( STDMETHODCALLTYPE *GetBytesWithSizePrefix )( 
            IBMDStreamingH264NALPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALPacket, GetDisplayTime)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayTime )( 
            IBMDStreamingH264NALPacket * This,
            /* [in] */ ULONGLONG requestedTimeScale,
            /* [out] */ ULONGLONG *displayTime);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALPacket, GetPacketIndex)
        HRESULT ( STDMETHODCALLTYPE *GetPacketIndex )( 
            IBMDStreamingH264NALPacket * This,
            /* [out] */ unsigned int *packetIndex);
        
        END_INTERFACE
    } IBMDStreamingH264NALPacketVtbl;

    interface IBMDStreamingH264NALPacket
    {
        CONST_VTBL struct IBMDStreamingH264NALPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingH264NALPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingH264NALPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingH264NALPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingH264NALPacket_GetPayloadSize(This)	\
    ( (This)->lpVtbl -> GetPayloadSize(This) ) 

#define IBMDStreamingH264NALPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IBMDStreamingH264NALPacket_GetBytesWithSizePrefix(This,buffer)	\
    ( (This)->lpVtbl -> GetBytesWithSizePrefix(This,buffer) ) 

#define IBMDStreamingH264NALPacket_GetDisplayTime(This,requestedTimeScale,displayTime)	\
    ( (This)->lpVtbl -> GetDisplayTime(This,requestedTimeScale,displayTime) ) 

#define IBMDStreamingH264NALPacket_GetPacketIndex(This,packetIndex)	\
    ( (This)->lpVtbl -> GetPacketIndex(This,packetIndex) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingH264NALPacket_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingAudioPacket_INTERFACE_DEFINED__
#define __IBMDStreamingAudioPacket_INTERFACE_DEFINED__

/* interface IBMDStreamingAudioPacket */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingAudioPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D9EB5902-1AD2-43F4-9E2C-3CFA50B5EE19")
    IBMDStreamingAudioPacket : public IUnknown
    {
    public:
        virtual BMDStreamingAudioCodec STDMETHODCALLTYPE GetCodec( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetPayloadSize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPlayTime( 
            /* [in] */ ULONGLONG requestedTimeScale,
            /* [out] */ ULONGLONG *playTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPacketIndex( 
            /* [out] */ unsigned int *packetIndex) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingAudioPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingAudioPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingAudioPacket, GetCodec)
        BMDStreamingAudioCodec ( STDMETHODCALLTYPE *GetCodec )( 
            IBMDStreamingAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingAudioPacket, GetPayloadSize)
        long ( STDMETHODCALLTYPE *GetPayloadSize )( 
            IBMDStreamingAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingAudioPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IBMDStreamingAudioPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IBMDStreamingAudioPacket, GetPlayTime)
        HRESULT ( STDMETHODCALLTYPE *GetPlayTime )( 
            IBMDStreamingAudioPacket * This,
            /* [in] */ ULONGLONG requestedTimeScale,
            /* [out] */ ULONGLONG *playTime);
        
        DECLSPEC_XFGVIRT(IBMDStreamingAudioPacket, GetPacketIndex)
        HRESULT ( STDMETHODCALLTYPE *GetPacketIndex )( 
            IBMDStreamingAudioPacket * This,
            /* [out] */ unsigned int *packetIndex);
        
        END_INTERFACE
    } IBMDStreamingAudioPacketVtbl;

    interface IBMDStreamingAudioPacket
    {
        CONST_VTBL struct IBMDStreamingAudioPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingAudioPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingAudioPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingAudioPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingAudioPacket_GetCodec(This)	\
    ( (This)->lpVtbl -> GetCodec(This) ) 

#define IBMDStreamingAudioPacket_GetPayloadSize(This)	\
    ( (This)->lpVtbl -> GetPayloadSize(This) ) 

#define IBMDStreamingAudioPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IBMDStreamingAudioPacket_GetPlayTime(This,requestedTimeScale,playTime)	\
    ( (This)->lpVtbl -> GetPlayTime(This,requestedTimeScale,playTime) ) 

#define IBMDStreamingAudioPacket_GetPacketIndex(This,packetIndex)	\
    ( (This)->lpVtbl -> GetPacketIndex(This,packetIndex) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingAudioPacket_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingMPEG2TSPacket_INTERFACE_DEFINED__
#define __IBMDStreamingMPEG2TSPacket_INTERFACE_DEFINED__

/* interface IBMDStreamingMPEG2TSPacket */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingMPEG2TSPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("91810D1C-4FB3-4AAA-AE56-FA301D3DFA4C")
    IBMDStreamingMPEG2TSPacket : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetPayloadSize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingMPEG2TSPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingMPEG2TSPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingMPEG2TSPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingMPEG2TSPacket * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMPEG2TSPacket, GetPayloadSize)
        long ( STDMETHODCALLTYPE *GetPayloadSize )( 
            IBMDStreamingMPEG2TSPacket * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingMPEG2TSPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IBMDStreamingMPEG2TSPacket * This,
            /* [out] */ void **buffer);
        
        END_INTERFACE
    } IBMDStreamingMPEG2TSPacketVtbl;

    interface IBMDStreamingMPEG2TSPacket
    {
        CONST_VTBL struct IBMDStreamingMPEG2TSPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingMPEG2TSPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingMPEG2TSPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingMPEG2TSPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingMPEG2TSPacket_GetPayloadSize(This)	\
    ( (This)->lpVtbl -> GetPayloadSize(This) ) 

#define IBMDStreamingMPEG2TSPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingMPEG2TSPacket_INTERFACE_DEFINED__ */


#ifndef __IBMDStreamingH264NALParser_INTERFACE_DEFINED__
#define __IBMDStreamingH264NALParser_INTERFACE_DEFINED__

/* interface IBMDStreamingH264NALParser */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMDStreamingH264NALParser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5867F18C-5BFA-4CCC-B2A7-9DFD140417D2")
    IBMDStreamingH264NALParser : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsNALSequenceParameterSet( 
            /* [in] */ IBMDStreamingH264NALPacket *nal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsNALPictureParameterSet( 
            /* [in] */ IBMDStreamingH264NALPacket *nal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProfileAndLevelFromSPS( 
            /* [in] */ IBMDStreamingH264NALPacket *nal,
            /* [out] */ unsigned int *profileIdc,
            /* [out] */ unsigned int *profileCompatability,
            /* [out] */ unsigned int *levelIdc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBMDStreamingH264NALParserVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBMDStreamingH264NALParser * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBMDStreamingH264NALParser * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBMDStreamingH264NALParser * This);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALParser, IsNALSequenceParameterSet)
        HRESULT ( STDMETHODCALLTYPE *IsNALSequenceParameterSet )( 
            IBMDStreamingH264NALParser * This,
            /* [in] */ IBMDStreamingH264NALPacket *nal);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALParser, IsNALPictureParameterSet)
        HRESULT ( STDMETHODCALLTYPE *IsNALPictureParameterSet )( 
            IBMDStreamingH264NALParser * This,
            /* [in] */ IBMDStreamingH264NALPacket *nal);
        
        DECLSPEC_XFGVIRT(IBMDStreamingH264NALParser, GetProfileAndLevelFromSPS)
        HRESULT ( STDMETHODCALLTYPE *GetProfileAndLevelFromSPS )( 
            IBMDStreamingH264NALParser * This,
            /* [in] */ IBMDStreamingH264NALPacket *nal,
            /* [out] */ unsigned int *profileIdc,
            /* [out] */ unsigned int *profileCompatability,
            /* [out] */ unsigned int *levelIdc);
        
        END_INTERFACE
    } IBMDStreamingH264NALParserVtbl;

    interface IBMDStreamingH264NALParser
    {
        CONST_VTBL struct IBMDStreamingH264NALParserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMDStreamingH264NALParser_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBMDStreamingH264NALParser_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBMDStreamingH264NALParser_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBMDStreamingH264NALParser_IsNALSequenceParameterSet(This,nal)	\
    ( (This)->lpVtbl -> IsNALSequenceParameterSet(This,nal) ) 

#define IBMDStreamingH264NALParser_IsNALPictureParameterSet(This,nal)	\
    ( (This)->lpVtbl -> IsNALPictureParameterSet(This,nal) ) 

#define IBMDStreamingH264NALParser_GetProfileAndLevelFromSPS(This,nal,profileIdc,profileCompatability,levelIdc)	\
    ( (This)->lpVtbl -> GetProfileAndLevelFromSPS(This,nal,profileIdc,profileCompatability,levelIdc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMDStreamingH264NALParser_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CBMDStreamingDiscovery;

#ifdef __cplusplus

class DECLSPEC_UUID("23A4EDF5-A0E5-432C-94EF-3BABB5F81C82")
CBMDStreamingDiscovery;
#endif

EXTERN_C const CLSID CLSID_CBMDStreamingH264NALParser;

#ifdef __cplusplus

class DECLSPEC_UUID("7753EFBD-951C-407C-97A5-23C737B73B52")
CBMDStreamingH264NALParser;
#endif

#ifndef __IDeckLinkVideoOutputCallback_INTERFACE_DEFINED__
#define __IDeckLinkVideoOutputCallback_INTERFACE_DEFINED__

/* interface IDeckLinkVideoOutputCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoOutputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5BE6DF26-02CE-433E-99D9-9A87C3AC171F")
    IDeckLinkVideoOutputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted( 
            /* [in] */ IDeckLinkVideoFrame *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoOutputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoOutputCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoOutputCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoOutputCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoOutputCallback, ScheduledFrameCompleted)
        HRESULT ( STDMETHODCALLTYPE *ScheduledFrameCompleted )( 
            IDeckLinkVideoOutputCallback * This,
            /* [in] */ IDeckLinkVideoFrame *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoOutputCallback, ScheduledPlaybackHasStopped)
        HRESULT ( STDMETHODCALLTYPE *ScheduledPlaybackHasStopped )( 
            IDeckLinkVideoOutputCallback * This);
        
        END_INTERFACE
    } IDeckLinkVideoOutputCallbackVtbl;

    interface IDeckLinkVideoOutputCallback
    {
        CONST_VTBL struct IDeckLinkVideoOutputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoOutputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoOutputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoOutputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoOutputCallback_ScheduledFrameCompleted(This,completedFrame,result)	\
    ( (This)->lpVtbl -> ScheduledFrameCompleted(This,completedFrame,result) ) 

#define IDeckLinkVideoOutputCallback_ScheduledPlaybackHasStopped(This)	\
    ( (This)->lpVtbl -> ScheduledPlaybackHasStopped(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoOutputCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInputCallback_INTERFACE_DEFINED__
#define __IDeckLinkInputCallback_INTERFACE_DEFINED__

/* interface IDeckLinkInputCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3A94F075-C37D-4BA8-BCC0-1D778C8F881B")
    IDeckLinkInputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged( 
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived( 
            /* [in] */ IDeckLinkVideoInputFrame *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInputCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInputCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInputCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInputCallback, VideoInputFormatChanged)
        HRESULT ( STDMETHODCALLTYPE *VideoInputFormatChanged )( 
            IDeckLinkInputCallback * This,
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInputCallback, VideoInputFrameArrived)
        HRESULT ( STDMETHODCALLTYPE *VideoInputFrameArrived )( 
            IDeckLinkInputCallback * This,
            /* [in] */ IDeckLinkVideoInputFrame *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket);
        
        END_INTERFACE
    } IDeckLinkInputCallbackVtbl;

    interface IDeckLinkInputCallback
    {
        CONST_VTBL struct IDeckLinkInputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInputCallback_VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags)	\
    ( (This)->lpVtbl -> VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags) ) 

#define IDeckLinkInputCallback_VideoInputFrameArrived(This,videoFrame,audioPacket)	\
    ( (This)->lpVtbl -> VideoInputFrameArrived(This,videoFrame,audioPacket) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInputCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderInputCallback_INTERFACE_DEFINED__
#define __IDeckLinkEncoderInputCallback_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderInputCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderInputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ACF13E61-F4A0-4974-A6A7-59AFF6268B31")
    IDeckLinkEncoderInputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VideoInputSignalChanged( 
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VideoPacketArrived( 
            /* [in] */ IDeckLinkEncoderVideoPacket *videoPacket) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AudioPacketArrived( 
            /* [in] */ IDeckLinkEncoderAudioPacket *audioPacket) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderInputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderInputCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderInputCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderInputCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInputCallback, VideoInputSignalChanged)
        HRESULT ( STDMETHODCALLTYPE *VideoInputSignalChanged )( 
            IDeckLinkEncoderInputCallback * This,
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInputCallback, VideoPacketArrived)
        HRESULT ( STDMETHODCALLTYPE *VideoPacketArrived )( 
            IDeckLinkEncoderInputCallback * This,
            /* [in] */ IDeckLinkEncoderVideoPacket *videoPacket);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInputCallback, AudioPacketArrived)
        HRESULT ( STDMETHODCALLTYPE *AudioPacketArrived )( 
            IDeckLinkEncoderInputCallback * This,
            /* [in] */ IDeckLinkEncoderAudioPacket *audioPacket);
        
        END_INTERFACE
    } IDeckLinkEncoderInputCallbackVtbl;

    interface IDeckLinkEncoderInputCallback
    {
        CONST_VTBL struct IDeckLinkEncoderInputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderInputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderInputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderInputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderInputCallback_VideoInputSignalChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags)	\
    ( (This)->lpVtbl -> VideoInputSignalChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags) ) 

#define IDeckLinkEncoderInputCallback_VideoPacketArrived(This,videoPacket)	\
    ( (This)->lpVtbl -> VideoPacketArrived(This,videoPacket) ) 

#define IDeckLinkEncoderInputCallback_AudioPacketArrived(This,audioPacket)	\
    ( (This)->lpVtbl -> AudioPacketArrived(This,audioPacket) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderInputCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoBufferAllocator_INTERFACE_DEFINED__
#define __IDeckLinkVideoBufferAllocator_INTERFACE_DEFINED__

/* interface IDeckLinkVideoBufferAllocator */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoBufferAllocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3481A4DF-2B11-4E55-AC61-836B87985E9A")
    IDeckLinkVideoBufferAllocator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AllocateVideoBuffer( 
            /* [out] */ IDeckLinkVideoBuffer **allocatedBuffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoBufferAllocatorVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoBufferAllocator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoBufferAllocator * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoBufferAllocator * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoBufferAllocator, AllocateVideoBuffer)
        HRESULT ( STDMETHODCALLTYPE *AllocateVideoBuffer )( 
            IDeckLinkVideoBufferAllocator * This,
            /* [out] */ IDeckLinkVideoBuffer **allocatedBuffer);
        
        END_INTERFACE
    } IDeckLinkVideoBufferAllocatorVtbl;

    interface IDeckLinkVideoBufferAllocator
    {
        CONST_VTBL struct IDeckLinkVideoBufferAllocatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoBufferAllocator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoBufferAllocator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoBufferAllocator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoBufferAllocator_AllocateVideoBuffer(This,allocatedBuffer)	\
    ( (This)->lpVtbl -> AllocateVideoBuffer(This,allocatedBuffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoBufferAllocator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoBufferAllocatorProvider_INTERFACE_DEFINED__
#define __IDeckLinkVideoBufferAllocatorProvider_INTERFACE_DEFINED__

/* interface IDeckLinkVideoBufferAllocatorProvider */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoBufferAllocatorProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08B80403-BFF2-49D0-B448-8C908B9E9FC9")
    IDeckLinkVideoBufferAllocatorProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetVideoBufferAllocator( 
            /* [in] */ unsigned int bufferSize,
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height,
            /* [in] */ unsigned int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoBufferAllocator **allocator) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoBufferAllocatorProviderVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoBufferAllocatorProvider * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoBufferAllocatorProvider * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoBufferAllocatorProvider * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoBufferAllocatorProvider, GetVideoBufferAllocator)
        HRESULT ( STDMETHODCALLTYPE *GetVideoBufferAllocator )( 
            IDeckLinkVideoBufferAllocatorProvider * This,
            /* [in] */ unsigned int bufferSize,
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height,
            /* [in] */ unsigned int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoBufferAllocator **allocator);
        
        END_INTERFACE
    } IDeckLinkVideoBufferAllocatorProviderVtbl;

    interface IDeckLinkVideoBufferAllocatorProvider
    {
        CONST_VTBL struct IDeckLinkVideoBufferAllocatorProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoBufferAllocatorProvider_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoBufferAllocatorProvider_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoBufferAllocatorProvider_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoBufferAllocatorProvider_GetVideoBufferAllocator(This,bufferSize,width,height,rowBytes,pixelFormat,allocator)	\
    ( (This)->lpVtbl -> GetVideoBufferAllocator(This,bufferSize,width,height,rowBytes,pixelFormat,allocator) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoBufferAllocatorProvider_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAudioOutputCallback_INTERFACE_DEFINED__
#define __IDeckLinkAudioOutputCallback_INTERFACE_DEFINED__

/* interface IDeckLinkAudioOutputCallback */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAudioOutputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("403C681B-7F46-4A12-B993-2BB127084EE6")
    IDeckLinkAudioOutputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples( 
            /* [in] */ BOOL preroll) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkAudioOutputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAudioOutputCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAudioOutputCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAudioOutputCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAudioOutputCallback, RenderAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *RenderAudioSamples )( 
            IDeckLinkAudioOutputCallback * This,
            /* [in] */ BOOL preroll);
        
        END_INTERFACE
    } IDeckLinkAudioOutputCallbackVtbl;

    interface IDeckLinkAudioOutputCallback
    {
        CONST_VTBL struct IDeckLinkAudioOutputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAudioOutputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAudioOutputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAudioOutputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAudioOutputCallback_RenderAudioSamples(This,preroll)	\
    ( (This)->lpVtbl -> RenderAudioSamples(This,preroll) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAudioOutputCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIterator_INTERFACE_DEFINED__
#define __IDeckLinkIterator_INTERFACE_DEFINED__

/* interface IDeckLinkIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("50FB36CD-3063-4B73-BDBB-958087F2D8BA")
    IDeckLinkIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLink **deckLinkInstance) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkIteratorVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIterator * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIterator * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIterator, Next)
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkIterator * This,
            /* [out] */ IDeckLink **deckLinkInstance);
        
        END_INTERFACE
    } IDeckLinkIteratorVtbl;

    interface IDeckLinkIterator
    {
        CONST_VTBL struct IDeckLinkIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIterator_Next(This,deckLinkInstance)	\
    ( (This)->lpVtbl -> Next(This,deckLinkInstance) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIterator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAPIInformation_INTERFACE_DEFINED__
#define __IDeckLinkAPIInformation_INTERFACE_DEFINED__

/* interface IDeckLinkAPIInformation */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAPIInformation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7BEA3C68-730D-4322-AF34-8A7152B532A4")
    IDeckLinkAPIInformation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkAPIInformationVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAPIInformation * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAPIInformation * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAPIInformation, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkAPIInformation, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkAPIInformation, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkAPIInformation, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkAPIInformationVtbl;

    interface IDeckLinkAPIInformation
    {
        CONST_VTBL struct IDeckLinkAPIInformationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAPIInformation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAPIInformation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAPIInformation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAPIInformation_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkAPIInformation_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkAPIInformation_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkAPIInformation_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAPIInformation_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIPFlowAttributes_INTERFACE_DEFINED__
#define __IDeckLinkIPFlowAttributes_INTERFACE_DEFINED__

/* interface IDeckLinkIPFlowAttributes */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIPFlowAttributes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CDA938DA-6479-40C6-B2EC-A3579B3AEECD")
    IDeckLinkIPFlowAttributes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkIPFlowAttributesVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIPFlowAttributes * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIPFlowAttributes * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIPFlowAttributes * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowAttributes, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkIPFlowAttributes * This,
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowAttributes, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkIPFlowAttributes * This,
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowAttributes, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkIPFlowAttributes * This,
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowAttributes, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkIPFlowAttributes * This,
            /* [in] */ BMDDeckLinkIPFlowAttributeID attrID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkIPFlowAttributesVtbl;

    interface IDeckLinkIPFlowAttributes
    {
        CONST_VTBL struct IDeckLinkIPFlowAttributesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIPFlowAttributes_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIPFlowAttributes_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIPFlowAttributes_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIPFlowAttributes_GetInt(This,attrID,value)	\
    ( (This)->lpVtbl -> GetInt(This,attrID,value) ) 

#define IDeckLinkIPFlowAttributes_GetFlag(This,attrID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,attrID,value) ) 

#define IDeckLinkIPFlowAttributes_GetFloat(This,attrID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,attrID,value) ) 

#define IDeckLinkIPFlowAttributes_GetString(This,attrID,value)	\
    ( (This)->lpVtbl -> GetString(This,attrID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIPFlowAttributes_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIPFlowStatus_INTERFACE_DEFINED__
#define __IDeckLinkIPFlowStatus_INTERFACE_DEFINED__

/* interface IDeckLinkIPFlowStatus */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIPFlowStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("31C41656-4992-4396-BBE9-5F8406AAB5AF")
    IDeckLinkIPFlowStatus : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkIPFlowStatusVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIPFlowStatus * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIPFlowStatus * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIPFlowStatus * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowStatus, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkIPFlowStatus * This,
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowStatus, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkIPFlowStatus * This,
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowStatus, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkIPFlowStatus * This,
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowStatus, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkIPFlowStatus * This,
            /* [in] */ BMDDeckLinkIPFlowStatusID statusID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkIPFlowStatusVtbl;

    interface IDeckLinkIPFlowStatus
    {
        CONST_VTBL struct IDeckLinkIPFlowStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIPFlowStatus_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIPFlowStatus_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIPFlowStatus_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIPFlowStatus_GetInt(This,statusID,value)	\
    ( (This)->lpVtbl -> GetInt(This,statusID,value) ) 

#define IDeckLinkIPFlowStatus_GetFlag(This,statusID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,statusID,value) ) 

#define IDeckLinkIPFlowStatus_GetFloat(This,statusID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,statusID,value) ) 

#define IDeckLinkIPFlowStatus_GetString(This,statusID,value)	\
    ( (This)->lpVtbl -> GetString(This,statusID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIPFlowStatus_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIPFlowSetting_INTERFACE_DEFINED__
#define __IDeckLinkIPFlowSetting_INTERFACE_DEFINED__

/* interface IDeckLinkIPFlowSetting */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIPFlowSetting;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("86DD9174-27D3-4032-B2AD-6067C3BB2424")
    IDeckLinkIPFlowSetting : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ BSTR value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkIPFlowSettingVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIPFlowSetting * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIPFlowSetting * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowSetting, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkIPFlowSetting * This,
            /* [in] */ BMDDeckLinkIPFlowSettingID settingID,
            /* [in] */ BSTR value);
        
        END_INTERFACE
    } IDeckLinkIPFlowSettingVtbl;

    interface IDeckLinkIPFlowSetting
    {
        CONST_VTBL struct IDeckLinkIPFlowSettingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIPFlowSetting_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIPFlowSetting_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIPFlowSetting_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIPFlowSetting_GetInt(This,settingID,value)	\
    ( (This)->lpVtbl -> GetInt(This,settingID,value) ) 

#define IDeckLinkIPFlowSetting_GetFlag(This,settingID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,settingID,value) ) 

#define IDeckLinkIPFlowSetting_GetFloat(This,settingID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,settingID,value) ) 

#define IDeckLinkIPFlowSetting_GetString(This,settingID,value)	\
    ( (This)->lpVtbl -> GetString(This,settingID,value) ) 

#define IDeckLinkIPFlowSetting_SetInt(This,settingID,value)	\
    ( (This)->lpVtbl -> SetInt(This,settingID,value) ) 

#define IDeckLinkIPFlowSetting_SetFlag(This,settingID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,settingID,value) ) 

#define IDeckLinkIPFlowSetting_SetFloat(This,settingID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,settingID,value) ) 

#define IDeckLinkIPFlowSetting_SetString(This,settingID,value)	\
    ( (This)->lpVtbl -> SetString(This,settingID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIPFlowSetting_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIPFlow_INTERFACE_DEFINED__
#define __IDeckLinkIPFlow_INTERFACE_DEFINED__

/* interface IDeckLinkIPFlow */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIPFlow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C5FC83C7-5B8E-42A7-9A40-7C065955D4E1")
    IDeckLinkIPFlow : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Enable( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disable( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkIPFlowVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIPFlow * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIPFlow * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIPFlow * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlow, Enable)
        HRESULT ( STDMETHODCALLTYPE *Enable )( 
            IDeckLinkIPFlow * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlow, Disable)
        HRESULT ( STDMETHODCALLTYPE *Disable )( 
            IDeckLinkIPFlow * This);
        
        END_INTERFACE
    } IDeckLinkIPFlowVtbl;

    interface IDeckLinkIPFlow
    {
        CONST_VTBL struct IDeckLinkIPFlowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIPFlow_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIPFlow_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIPFlow_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIPFlow_Enable(This)	\
    ( (This)->lpVtbl -> Enable(This) ) 

#define IDeckLinkIPFlow_Disable(This)	\
    ( (This)->lpVtbl -> Disable(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIPFlow_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIPFlowIterator_INTERFACE_DEFINED__
#define __IDeckLinkIPFlowIterator_INTERFACE_DEFINED__

/* interface IDeckLinkIPFlowIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIPFlowIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BD296AB2-A5C5-4153-888F-AAB1FDBD8A5C")
    IDeckLinkIPFlowIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLinkIPFlow **deckLinkIPFlowInstance) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkIPFlowIteratorVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIPFlowIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIPFlowIterator * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIPFlowIterator * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPFlowIterator, Next)
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkIPFlowIterator * This,
            /* [out] */ IDeckLinkIPFlow **deckLinkIPFlowInstance);
        
        END_INTERFACE
    } IDeckLinkIPFlowIteratorVtbl;

    interface IDeckLinkIPFlowIterator
    {
        CONST_VTBL struct IDeckLinkIPFlowIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIPFlowIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIPFlowIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIPFlowIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIPFlowIterator_Next(This,deckLinkIPFlowInstance)	\
    ( (This)->lpVtbl -> Next(This,deckLinkIPFlowInstance) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIPFlowIterator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkOutput_INTERFACE_DEFINED__
#define __IDeckLinkOutput_INTERFACE_DEFINED__

/* interface IDeckLinkOutput */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkOutput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1A8077F1-9FE2-4533-8147-2294305E253F")
    IDeckLinkOutput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoOutputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoOutput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrame( 
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrameWithBuffer( 
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [in] */ IDeckLinkVideoBuffer *buffer,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RowBytesForPixelFormat( 
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ int width,
            /* [out] */ int *rowBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAncillaryData( 
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayVideoFrameSync( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleVideoFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduledFrameCompletionCallback( 
            /* [in] */ IDeckLinkVideoOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedVideoFrameCount( 
            /* [out] */ unsigned int *bufferedFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioOutput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAudioSamplesSync( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleAudioSamples( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned int *bufferedSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushBufferedAudioSamples( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioCallback( 
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartScheduledPlayback( 
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopScheduledPlayback( 
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsScheduledPlaybackRunning( 
            /* [out] */ BOOL *active) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScheduledStreamTime( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferenceStatus( 
            /* [out] */ BMDReferenceStatus *referenceStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameCompletionReferenceTimestamp( 
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkOutputVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkOutput * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkOutput * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkOutput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoOutputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkOutput * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, EnableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoOutput )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, DisableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoOutput )( 
            IDeckLinkOutput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, CreateVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrame )( 
            IDeckLinkOutput * This,
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, CreateVideoFrameWithBuffer)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrameWithBuffer )( 
            IDeckLinkOutput * This,
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [in] */ IDeckLinkVideoBuffer *buffer,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, RowBytesForPixelFormat)
        HRESULT ( STDMETHODCALLTYPE *RowBytesForPixelFormat )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ int width,
            /* [out] */ int *rowBytes);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, CreateAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *CreateAncillaryData )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, DisplayVideoFrameSync)
        HRESULT ( STDMETHODCALLTYPE *DisplayVideoFrameSync )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, ScheduleVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *ScheduleVideoFrame )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, SetScheduledFrameCompletionCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScheduledFrameCompletionCallback )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkVideoOutputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetBufferedVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedVideoFrameCount )( 
            IDeckLinkOutput * This,
            /* [out] */ unsigned int *bufferedFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, EnableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioOutput )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, DisableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioOutput )( 
            IDeckLinkOutput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, WriteAudioSamplesSync)
        HRESULT ( STDMETHODCALLTYPE *WriteAudioSamplesSync )( 
            IDeckLinkOutput * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, BeginAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *BeginAudioPreroll )( 
            IDeckLinkOutput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, EndAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *EndAudioPreroll )( 
            IDeckLinkOutput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, ScheduleAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *ScheduleAudioSamples )( 
            IDeckLinkOutput * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetBufferedAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkOutput * This,
            /* [out] */ unsigned int *bufferedSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, FlushBufferedAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *FlushBufferedAudioSamples )( 
            IDeckLinkOutput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, SetAudioCallback)
        HRESULT ( STDMETHODCALLTYPE *SetAudioCallback )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, StartScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StartScheduledPlayback )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, StopScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StopScheduledPlayback )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, IsScheduledPlaybackRunning)
        HRESULT ( STDMETHODCALLTYPE *IsScheduledPlaybackRunning )( 
            IDeckLinkOutput * This,
            /* [out] */ BOOL *active);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetScheduledStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetScheduledStreamTime )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetReferenceStatus)
        HRESULT ( STDMETHODCALLTYPE *GetReferenceStatus )( 
            IDeckLinkOutput * This,
            /* [out] */ BMDReferenceStatus *referenceStatus);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkOutput * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput, GetFrameCompletionReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetFrameCompletionReferenceTimestamp )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp);
        
        END_INTERFACE
    } IDeckLinkOutputVtbl;

    interface IDeckLinkOutput
    {
        CONST_VTBL struct IDeckLinkOutputVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkOutput_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkOutput_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkOutput_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkOutput_DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported) ) 

#define IDeckLinkOutput_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkOutput_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkOutput_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkOutput_EnableVideoOutput(This,displayMode,flags)	\
    ( (This)->lpVtbl -> EnableVideoOutput(This,displayMode,flags) ) 

#define IDeckLinkOutput_DisableVideoOutput(This)	\
    ( (This)->lpVtbl -> DisableVideoOutput(This) ) 

#define IDeckLinkOutput_CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_CreateVideoFrameWithBuffer(This,width,height,rowBytes,pixelFormat,flags,buffer,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrameWithBuffer(This,width,height,rowBytes,pixelFormat,flags,buffer,outFrame) ) 

#define IDeckLinkOutput_RowBytesForPixelFormat(This,pixelFormat,width,rowBytes)	\
    ( (This)->lpVtbl -> RowBytesForPixelFormat(This,pixelFormat,width,rowBytes) ) 

#define IDeckLinkOutput_CreateAncillaryData(This,pixelFormat,outBuffer)	\
    ( (This)->lpVtbl -> CreateAncillaryData(This,pixelFormat,outBuffer) ) 

#define IDeckLinkOutput_DisplayVideoFrameSync(This,theFrame)	\
    ( (This)->lpVtbl -> DisplayVideoFrameSync(This,theFrame) ) 

#define IDeckLinkOutput_ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale)	\
    ( (This)->lpVtbl -> ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale) ) 

#define IDeckLinkOutput_SetScheduledFrameCompletionCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetScheduledFrameCompletionCallback(This,theCallback) ) 

#define IDeckLinkOutput_GetBufferedVideoFrameCount(This,bufferedFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedVideoFrameCount(This,bufferedFrameCount) ) 

#define IDeckLinkOutput_EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType)	\
    ( (This)->lpVtbl -> EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType) ) 

#define IDeckLinkOutput_DisableAudioOutput(This)	\
    ( (This)->lpVtbl -> DisableAudioOutput(This) ) 

#define IDeckLinkOutput_WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten)	\
    ( (This)->lpVtbl -> WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten) ) 

#define IDeckLinkOutput_BeginAudioPreroll(This)	\
    ( (This)->lpVtbl -> BeginAudioPreroll(This) ) 

#define IDeckLinkOutput_EndAudioPreroll(This)	\
    ( (This)->lpVtbl -> EndAudioPreroll(This) ) 

#define IDeckLinkOutput_ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten)	\
    ( (This)->lpVtbl -> ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten) ) 

#define IDeckLinkOutput_GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount) ) 

#define IDeckLinkOutput_FlushBufferedAudioSamples(This)	\
    ( (This)->lpVtbl -> FlushBufferedAudioSamples(This) ) 

#define IDeckLinkOutput_SetAudioCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetAudioCallback(This,theCallback) ) 

#define IDeckLinkOutput_StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed)	\
    ( (This)->lpVtbl -> StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed) ) 

#define IDeckLinkOutput_StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale)	\
    ( (This)->lpVtbl -> StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale) ) 

#define IDeckLinkOutput_IsScheduledPlaybackRunning(This,active)	\
    ( (This)->lpVtbl -> IsScheduledPlaybackRunning(This,active) ) 

#define IDeckLinkOutput_GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed)	\
    ( (This)->lpVtbl -> GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed) ) 

#define IDeckLinkOutput_GetReferenceStatus(This,referenceStatus)	\
    ( (This)->lpVtbl -> GetReferenceStatus(This,referenceStatus) ) 

#define IDeckLinkOutput_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#define IDeckLinkOutput_GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp)	\
    ( (This)->lpVtbl -> GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkOutput_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_INTERFACE_DEFINED__
#define __IDeckLinkInput_INTERFACE_DEFINED__

/* interface IDeckLinkInput */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4095DB82-E294-4B8C-AAA8-3B9E80C49336")
    IDeckLinkInput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoInputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInputWithAllocatorProvider( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags,
            /* [in] */ IDeckLinkVideoBufferAllocatorProvider *allocatorProvider) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoFrameCount( 
            /* [out] */ unsigned int *availableFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoInputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkInput * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkInput * This,
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, EnableVideoInputWithAllocatorProvider)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInputWithAllocatorProvider )( 
            IDeckLinkInput * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags,
            /* [in] */ IDeckLinkVideoBufferAllocatorProvider *allocatorProvider);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, GetAvailableVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableVideoFrameCount )( 
            IDeckLinkInput * This,
            /* [out] */ unsigned int *availableFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkInput * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput * This,
            /* [in] */ IDeckLinkInputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkInput * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkInputVtbl;

    interface IDeckLinkInput
    {
        CONST_VTBL struct IDeckLinkInputVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported) ) 

#define IDeckLinkInput_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkInput_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkInput_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_EnableVideoInputWithAllocatorProvider(This,displayMode,pixelFormat,flags,allocatorProvider)	\
    ( (This)->lpVtbl -> EnableVideoInputWithAllocatorProvider(This,displayMode,pixelFormat,flags,allocatorProvider) ) 

#define IDeckLinkInput_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_GetAvailableVideoFrameCount(This,availableFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableVideoFrameCount(This,availableFrameCount) ) 

#define IDeckLinkInput_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkInput_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkInput_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkInput_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIPExtensions_INTERFACE_DEFINED__
#define __IDeckLinkIPExtensions_INTERFACE_DEFINED__

/* interface IDeckLinkIPExtensions */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIPExtensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("46CF7903-A9FD-4D0B-8FFC-0103722AB442")
    IDeckLinkIPExtensions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDeckLinkIPFlowIterator( 
            /* [out] */ IDeckLinkIPFlowIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIPFlowByID( 
            /* [in] */ BMDIPFlowID id,
            /* [out] */ IDeckLinkIPFlow **flow) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkIPExtensionsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIPExtensions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIPExtensions * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIPExtensions * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPExtensions, GetDeckLinkIPFlowIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDeckLinkIPFlowIterator )( 
            IDeckLinkIPExtensions * This,
            /* [out] */ IDeckLinkIPFlowIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkIPExtensions, GetIPFlowByID)
        HRESULT ( STDMETHODCALLTYPE *GetIPFlowByID )( 
            IDeckLinkIPExtensions * This,
            /* [in] */ BMDIPFlowID id,
            /* [out] */ IDeckLinkIPFlow **flow);
        
        END_INTERFACE
    } IDeckLinkIPExtensionsVtbl;

    interface IDeckLinkIPExtensions
    {
        CONST_VTBL struct IDeckLinkIPExtensionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIPExtensions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIPExtensions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIPExtensions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIPExtensions_GetDeckLinkIPFlowIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDeckLinkIPFlowIterator(This,iterator) ) 

#define IDeckLinkIPExtensions_GetIPFlowByID(This,id,flow)	\
    ( (This)->lpVtbl -> GetIPFlowByID(This,id,flow) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIPExtensions_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkHDMIInputEDID_INTERFACE_DEFINED__
#define __IDeckLinkHDMIInputEDID_INTERFACE_DEFINED__

/* interface IDeckLinkHDMIInputEDID */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkHDMIInputEDID;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ABBBACBC-45BC-4665-9D92-ACE6E5A97902")
    IDeckLinkHDMIInputEDID : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkHDMIInputEDIDID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkHDMIInputEDIDID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteToEDID( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkHDMIInputEDIDVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkHDMIInputEDID * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkHDMIInputEDID * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkHDMIInputEDID * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkHDMIInputEDID, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkHDMIInputEDID * This,
            /* [in] */ BMDDeckLinkHDMIInputEDIDID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkHDMIInputEDID, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkHDMIInputEDID * This,
            /* [in] */ BMDDeckLinkHDMIInputEDIDID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkHDMIInputEDID, WriteToEDID)
        HRESULT ( STDMETHODCALLTYPE *WriteToEDID )( 
            IDeckLinkHDMIInputEDID * This);
        
        END_INTERFACE
    } IDeckLinkHDMIInputEDIDVtbl;

    interface IDeckLinkHDMIInputEDID
    {
        CONST_VTBL struct IDeckLinkHDMIInputEDIDVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkHDMIInputEDID_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkHDMIInputEDID_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkHDMIInputEDID_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkHDMIInputEDID_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkHDMIInputEDID_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkHDMIInputEDID_WriteToEDID(This)	\
    ( (This)->lpVtbl -> WriteToEDID(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkHDMIInputEDID_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderInput_INTERFACE_DEFINED__
#define __IDeckLinkEncoderInput_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderInput */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderInput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("46C1332E-6FD9-472A-8591-FE59C22192E1")
    IDeckLinkEncoderInput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedCodec,
            /* [in] */ unsigned int requestedCodecProfile,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailablePacketsCount( 
            /* [out] */ unsigned int *availablePacketsCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioFormat audioFormat,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkEncoderInputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderInputVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderInput * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkEncoderInput * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedCodec,
            /* [in] */ unsigned int requestedCodecProfile,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkEncoderInput * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkEncoderInput * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkEncoderInput * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, GetAvailablePacketsCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailablePacketsCount )( 
            IDeckLinkEncoderInput * This,
            /* [out] */ unsigned int *availablePacketsCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkEncoderInput * This,
            /* [in] */ BMDAudioFormat audioFormat,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkEncoderInput * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkEncoderInput * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkEncoderInput * This,
            /* [in] */ IDeckLinkEncoderInputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkEncoderInput * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkEncoderInputVtbl;

    interface IDeckLinkEncoderInput
    {
        CONST_VTBL struct IDeckLinkEncoderInputVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderInput_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderInput_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderInput_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderInput_DoesSupportVideoMode(This,connection,requestedMode,requestedCodec,requestedCodecProfile,flags,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedCodec,requestedCodecProfile,flags,supported) ) 

#define IDeckLinkEncoderInput_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkEncoderInput_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkEncoderInput_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkEncoderInput_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkEncoderInput_GetAvailablePacketsCount(This,availablePacketsCount)	\
    ( (This)->lpVtbl -> GetAvailablePacketsCount(This,availablePacketsCount) ) 

#define IDeckLinkEncoderInput_EnableAudioInput(This,audioFormat,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,audioFormat,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkEncoderInput_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkEncoderInput_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkEncoderInput_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkEncoderInput_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkEncoderInput_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkEncoderInput_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkEncoderInput_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkEncoderInput_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderInput_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoBuffer_INTERFACE_DEFINED__
#define __IDeckLinkVideoBuffer_INTERFACE_DEFINED__

/* interface IDeckLinkVideoBuffer */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoBuffer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CCB4B64A-5C86-4E02-B778-885D352709FE")
    IDeckLinkVideoBuffer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartAccess( 
            /* [in] */ BMDBufferAccessFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAccess( 
            /* [in] */ BMDBufferAccessFlags flags) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoBufferVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoBuffer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoBuffer * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoBuffer * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoBuffer, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoBuffer * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoBuffer, StartAccess)
        HRESULT ( STDMETHODCALLTYPE *StartAccess )( 
            IDeckLinkVideoBuffer * This,
            /* [in] */ BMDBufferAccessFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoBuffer, EndAccess)
        HRESULT ( STDMETHODCALLTYPE *EndAccess )( 
            IDeckLinkVideoBuffer * This,
            /* [in] */ BMDBufferAccessFlags flags);
        
        END_INTERFACE
    } IDeckLinkVideoBufferVtbl;

    interface IDeckLinkVideoBuffer
    {
        CONST_VTBL struct IDeckLinkVideoBufferVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoBuffer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoBuffer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoBuffer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoBuffer_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkVideoBuffer_StartAccess(This,flags)	\
    ( (This)->lpVtbl -> StartAccess(This,flags) ) 

#define IDeckLinkVideoBuffer_EndAccess(This,flags)	\
    ( (This)->lpVtbl -> EndAccess(This,flags) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoBuffer_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrame_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrame */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6502091C-615F-4F51-BAF6-45C4256DD5B0")
    IDeckLinkVideoFrame : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetWidth( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetHeight( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetRowBytes( void) = 0;
        
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void) = 0;
        
        virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecode( 
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAncillaryData( 
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrame * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetWidth)
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetHeight)
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetRowBytes)
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetFlags)
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkVideoFrame * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkVideoFrame * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        END_INTERFACE
    } IDeckLinkVideoFrameVtbl;

    interface IDeckLinkVideoFrame
    {
        CONST_VTBL struct IDeckLinkVideoFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrame_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoFrame_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoFrame_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoFrame_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoFrame_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoFrame_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkVideoFrame_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrame_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkMutableVideoFrame_INTERFACE_DEFINED__
#define __IDeckLinkMutableVideoFrame_INTERFACE_DEFINED__

/* interface IDeckLinkMutableVideoFrame */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkMutableVideoFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CF9EB134-0374-4C5B-95FA-1EC14819FF62")
    IDeckLinkMutableVideoFrame : public IDeckLinkVideoFrame
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlags( 
            /* [in] */ BMDFrameFlags newFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecode( 
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ IDeckLinkTimecode *timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecodeFromComponents( 
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ unsigned char hours,
            /* [in] */ unsigned char minutes,
            /* [in] */ unsigned char seconds,
            /* [in] */ unsigned char frames,
            /* [in] */ BMDTimecodeFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAncillaryData( 
            /* [in] */ IDeckLinkVideoFrameAncillary *ancillary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecodeUserBits( 
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ BMDTimecodeUserBits userBits) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInterfaceProvider( 
            /* [in] */ REFIID iid,
            /* [in] */ IUnknown *iface) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkMutableVideoFrameVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkMutableVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkMutableVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetWidth)
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkMutableVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetHeight)
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkMutableVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetRowBytes)
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkMutableVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkMutableVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetFlags)
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkMutableVideoFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkMutableVideoFrame * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame, SetFlags)
        HRESULT ( STDMETHODCALLTYPE *SetFlags )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ BMDFrameFlags newFlags);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame, SetTimecode)
        HRESULT ( STDMETHODCALLTYPE *SetTimecode )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ IDeckLinkTimecode *timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame, SetTimecodeFromComponents)
        HRESULT ( STDMETHODCALLTYPE *SetTimecodeFromComponents )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ unsigned char hours,
            /* [in] */ unsigned char minutes,
            /* [in] */ unsigned char seconds,
            /* [in] */ unsigned char frames,
            /* [in] */ BMDTimecodeFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame, SetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *SetAncillaryData )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ IDeckLinkVideoFrameAncillary *ancillary);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame, SetTimecodeUserBits)
        HRESULT ( STDMETHODCALLTYPE *SetTimecodeUserBits )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ BMDTimecodeUserBits userBits);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame, SetInterfaceProvider)
        HRESULT ( STDMETHODCALLTYPE *SetInterfaceProvider )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ REFIID iid,
            /* [in] */ IUnknown *iface);
        
        END_INTERFACE
    } IDeckLinkMutableVideoFrameVtbl;

    interface IDeckLinkMutableVideoFrame
    {
        CONST_VTBL struct IDeckLinkMutableVideoFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkMutableVideoFrame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkMutableVideoFrame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkMutableVideoFrame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkMutableVideoFrame_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkMutableVideoFrame_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkMutableVideoFrame_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkMutableVideoFrame_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkMutableVideoFrame_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkMutableVideoFrame_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkMutableVideoFrame_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 


#define IDeckLinkMutableVideoFrame_SetFlags(This,newFlags)	\
    ( (This)->lpVtbl -> SetFlags(This,newFlags) ) 

#define IDeckLinkMutableVideoFrame_SetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> SetTimecode(This,format,timecode) ) 

#define IDeckLinkMutableVideoFrame_SetTimecodeFromComponents(This,format,hours,minutes,seconds,frames,flags)	\
    ( (This)->lpVtbl -> SetTimecodeFromComponents(This,format,hours,minutes,seconds,frames,flags) ) 

#define IDeckLinkMutableVideoFrame_SetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> SetAncillaryData(This,ancillary) ) 

#define IDeckLinkMutableVideoFrame_SetTimecodeUserBits(This,format,userBits)	\
    ( (This)->lpVtbl -> SetTimecodeUserBits(This,format,userBits) ) 

#define IDeckLinkMutableVideoFrame_SetInterfaceProvider(This,iid,iface)	\
    ( (This)->lpVtbl -> SetInterfaceProvider(This,iid,iface) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkMutableVideoFrame_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrame3DExtensions_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrame3DExtensions_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrame3DExtensions */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrame3DExtensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D4DBE9C6-B4D2-49D3-ABF2-B4E86C7391B0")
    IDeckLinkVideoFrame3DExtensions : public IUnknown
    {
    public:
        virtual BMDVideo3DPackingFormat STDMETHODCALLTYPE Get3DPackingFormat( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameForRightEye( 
            /* [out] */ IDeckLinkVideoFrame **rightEyeFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrame3DExtensionsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrame3DExtensions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrame3DExtensions * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrame3DExtensions * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame3DExtensions, Get3DPackingFormat)
        BMDVideo3DPackingFormat ( STDMETHODCALLTYPE *Get3DPackingFormat )( 
            IDeckLinkVideoFrame3DExtensions * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame3DExtensions, GetFrameForRightEye)
        HRESULT ( STDMETHODCALLTYPE *GetFrameForRightEye )( 
            IDeckLinkVideoFrame3DExtensions * This,
            /* [out] */ IDeckLinkVideoFrame **rightEyeFrame);
        
        END_INTERFACE
    } IDeckLinkVideoFrame3DExtensionsVtbl;

    interface IDeckLinkVideoFrame3DExtensions
    {
        CONST_VTBL struct IDeckLinkVideoFrame3DExtensionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrame3DExtensions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrame3DExtensions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrame3DExtensions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrame3DExtensions_Get3DPackingFormat(This)	\
    ( (This)->lpVtbl -> Get3DPackingFormat(This) ) 

#define IDeckLinkVideoFrame3DExtensions_GetFrameForRightEye(This,rightEyeFrame)	\
    ( (This)->lpVtbl -> GetFrameForRightEye(This,rightEyeFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrame3DExtensions_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrameMetadataExtensions_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrameMetadataExtensions_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrameMetadataExtensions */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrameMetadataExtensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E232A5B7-4DB4-44C9-9152-F47C12E5F051")
    IDeckLinkVideoFrameMetadataExtensions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ void *buffer,
            /* [out][in] */ unsigned int *bufferSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameMetadataExtensionsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrameMetadataExtensions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrameMetadataExtensions * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrameMetadataExtensions * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkVideoFrameMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkVideoFrameMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkVideoFrameMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkVideoFrameMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoFrameMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ void *buffer,
            /* [out][in] */ unsigned int *bufferSize);
        
        END_INTERFACE
    } IDeckLinkVideoFrameMetadataExtensionsVtbl;

    interface IDeckLinkVideoFrameMetadataExtensions
    {
        CONST_VTBL struct IDeckLinkVideoFrameMetadataExtensionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrameMetadataExtensions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrameMetadataExtensions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrameMetadataExtensions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrameMetadataExtensions_GetInt(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetInt(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMetadataExtensions_GetFloat(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMetadataExtensions_GetFlag(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMetadataExtensions_GetString(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetString(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMetadataExtensions_GetBytes(This,metadataID,buffer,bufferSize)	\
    ( (This)->lpVtbl -> GetBytes(This,metadataID,buffer,bufferSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrameMetadataExtensions_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrameMutableMetadataExtensions_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrameMutableMetadataExtensions_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrameMutableMetadataExtensions */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrameMutableMetadataExtensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC198FC6-8298-4419-942D-8357EC355E58")
    IDeckLinkVideoFrameMutableMetadataExtensions : public IDeckLinkVideoFrameMetadataExtensions
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBytes( 
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int bufferSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameMutableMetadataExtensionsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [out] */ void *buffer,
            /* [out][in] */ unsigned int *bufferSize);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMutableMetadataExtensions, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMutableMetadataExtensions, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMutableMetadataExtensions, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMutableMetadataExtensions, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMutableMetadataExtensions, SetBytes)
        HRESULT ( STDMETHODCALLTYPE *SetBytes )( 
            IDeckLinkVideoFrameMutableMetadataExtensions * This,
            /* [in] */ BMDDeckLinkFrameMetadataID metadataID,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int bufferSize);
        
        END_INTERFACE
    } IDeckLinkVideoFrameMutableMetadataExtensionsVtbl;

    interface IDeckLinkVideoFrameMutableMetadataExtensions
    {
        CONST_VTBL struct IDeckLinkVideoFrameMutableMetadataExtensionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrameMutableMetadataExtensions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrameMutableMetadataExtensions_GetInt(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetInt(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_GetFloat(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_GetFlag(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_GetString(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetString(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_GetBytes(This,metadataID,buffer,bufferSize)	\
    ( (This)->lpVtbl -> GetBytes(This,metadataID,buffer,bufferSize) ) 


#define IDeckLinkVideoFrameMutableMetadataExtensions_SetInt(This,metadataID,value)	\
    ( (This)->lpVtbl -> SetInt(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_SetFloat(This,metadataID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_SetFlag(This,metadataID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_SetString(This,metadataID,value)	\
    ( (This)->lpVtbl -> SetString(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMutableMetadataExtensions_SetBytes(This,metadataID,buffer,bufferSize)	\
    ( (This)->lpVtbl -> SetBytes(This,metadataID,buffer,bufferSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrameMutableMetadataExtensions_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_INTERFACE_DEFINED__
#define __IDeckLinkVideoInputFrame_INTERFACE_DEFINED__

/* interface IDeckLinkVideoInputFrame */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoInputFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C9ADD3D2-BE52-488D-AB2D-7FDEF7AF0C95")
    IDeckLinkVideoInputFrame : public IDeckLinkVideoFrame
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStreamTime( 
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceTimestamp( 
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoInputFrameVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoInputFrame * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoInputFrame * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoInputFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetWidth)
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoInputFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetHeight)
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoInputFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetRowBytes)
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoInputFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoInputFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetFlags)
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoInputFrame * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkVideoInputFrame * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame, GetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkVideoInputFrame * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoInputFrame, GetStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkVideoInputFrame * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoInputFrame, GetHardwareReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceTimestamp )( 
            IDeckLinkVideoInputFrame * This,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration);
        
        END_INTERFACE
    } IDeckLinkVideoInputFrameVtbl;

    interface IDeckLinkVideoInputFrame
    {
        CONST_VTBL struct IDeckLinkVideoInputFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoInputFrame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoInputFrame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoInputFrame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoInputFrame_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoInputFrame_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoInputFrame_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoInputFrame_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoInputFrame_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoInputFrame_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkVideoInputFrame_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 


#define IDeckLinkVideoInputFrame_GetStreamTime(This,frameTime,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,frameDuration,timeScale) ) 

#define IDeckLinkVideoInputFrame_GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration)	\
    ( (This)->lpVtbl -> GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoInputFrame_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAncillaryPacket_INTERFACE_DEFINED__
#define __IDeckLinkAncillaryPacket_INTERFACE_DEFINED__

/* interface IDeckLinkAncillaryPacket */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAncillaryPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC5BBF7E-029C-4D3B-9158-6000EF5E3670")
    IDeckLinkAncillaryPacket : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [in] */ BMDAncillaryPacketFormat format,
            /* [out] */ const void **data,
            /* [out] */ unsigned int *size) = 0;
        
        virtual unsigned char STDMETHODCALLTYPE GetDID( void) = 0;
        
        virtual unsigned char STDMETHODCALLTYPE GetSDID( void) = 0;
        
        virtual unsigned int STDMETHODCALLTYPE GetLineNumber( void) = 0;
        
        virtual unsigned char STDMETHODCALLTYPE GetDataStreamIndex( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkAncillaryPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAncillaryPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAncillaryPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAncillaryPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAncillaryPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkAncillaryPacket * This,
            /* [in] */ BMDAncillaryPacketFormat format,
            /* [out] */ const void **data,
            /* [out] */ unsigned int *size);
        
        DECLSPEC_XFGVIRT(IDeckLinkAncillaryPacket, GetDID)
        unsigned char ( STDMETHODCALLTYPE *GetDID )( 
            IDeckLinkAncillaryPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAncillaryPacket, GetSDID)
        unsigned char ( STDMETHODCALLTYPE *GetSDID )( 
            IDeckLinkAncillaryPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAncillaryPacket, GetLineNumber)
        unsigned int ( STDMETHODCALLTYPE *GetLineNumber )( 
            IDeckLinkAncillaryPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAncillaryPacket, GetDataStreamIndex)
        unsigned char ( STDMETHODCALLTYPE *GetDataStreamIndex )( 
            IDeckLinkAncillaryPacket * This);
        
        END_INTERFACE
    } IDeckLinkAncillaryPacketVtbl;

    interface IDeckLinkAncillaryPacket
    {
        CONST_VTBL struct IDeckLinkAncillaryPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAncillaryPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAncillaryPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAncillaryPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAncillaryPacket_GetBytes(This,format,data,size)	\
    ( (This)->lpVtbl -> GetBytes(This,format,data,size) ) 

#define IDeckLinkAncillaryPacket_GetDID(This)	\
    ( (This)->lpVtbl -> GetDID(This) ) 

#define IDeckLinkAncillaryPacket_GetSDID(This)	\
    ( (This)->lpVtbl -> GetSDID(This) ) 

#define IDeckLinkAncillaryPacket_GetLineNumber(This)	\
    ( (This)->lpVtbl -> GetLineNumber(This) ) 

#define IDeckLinkAncillaryPacket_GetDataStreamIndex(This)	\
    ( (This)->lpVtbl -> GetDataStreamIndex(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAncillaryPacket_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAncillaryPacketIterator_INTERFACE_DEFINED__
#define __IDeckLinkAncillaryPacketIterator_INTERFACE_DEFINED__

/* interface IDeckLinkAncillaryPacketIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAncillaryPacketIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3FC8994B-88FB-4C17-968F-9AAB69D964A7")
    IDeckLinkAncillaryPacketIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLinkAncillaryPacket **packet) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkAncillaryPacketIteratorVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAncillaryPacketIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAncillaryPacketIterator * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAncillaryPacketIterator * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAncillaryPacketIterator, Next)
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkAncillaryPacketIterator * This,
            /* [out] */ IDeckLinkAncillaryPacket **packet);
        
        END_INTERFACE
    } IDeckLinkAncillaryPacketIteratorVtbl;

    interface IDeckLinkAncillaryPacketIterator
    {
        CONST_VTBL struct IDeckLinkAncillaryPacketIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAncillaryPacketIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAncillaryPacketIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAncillaryPacketIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAncillaryPacketIterator_Next(This,packet)	\
    ( (This)->lpVtbl -> Next(This,packet) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAncillaryPacketIterator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrameAncillaryPackets_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrameAncillaryPackets_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrameAncillaryPackets */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrameAncillaryPackets;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6C186C0F-459E-41D8-AEE2-4812D81AEE68")
    IDeckLinkVideoFrameAncillaryPackets : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPacketIterator( 
            /* [out] */ IDeckLinkAncillaryPacketIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFirstPacketByID( 
            /* [in] */ unsigned char DID,
            /* [in] */ unsigned char SDID,
            /* [out] */ IDeckLinkAncillaryPacket **packet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AttachPacket( 
            /* [in] */ IDeckLinkAncillaryPacket *packet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DetachPacket( 
            /* [in] */ IDeckLinkAncillaryPacket *packet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DetachAllPackets( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameAncillaryPacketsVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrameAncillaryPackets * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrameAncillaryPackets * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrameAncillaryPackets * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillaryPackets, GetPacketIterator)
        HRESULT ( STDMETHODCALLTYPE *GetPacketIterator )( 
            IDeckLinkVideoFrameAncillaryPackets * This,
            /* [out] */ IDeckLinkAncillaryPacketIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillaryPackets, GetFirstPacketByID)
        HRESULT ( STDMETHODCALLTYPE *GetFirstPacketByID )( 
            IDeckLinkVideoFrameAncillaryPackets * This,
            /* [in] */ unsigned char DID,
            /* [in] */ unsigned char SDID,
            /* [out] */ IDeckLinkAncillaryPacket **packet);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillaryPackets, AttachPacket)
        HRESULT ( STDMETHODCALLTYPE *AttachPacket )( 
            IDeckLinkVideoFrameAncillaryPackets * This,
            /* [in] */ IDeckLinkAncillaryPacket *packet);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillaryPackets, DetachPacket)
        HRESULT ( STDMETHODCALLTYPE *DetachPacket )( 
            IDeckLinkVideoFrameAncillaryPackets * This,
            /* [in] */ IDeckLinkAncillaryPacket *packet);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillaryPackets, DetachAllPackets)
        HRESULT ( STDMETHODCALLTYPE *DetachAllPackets )( 
            IDeckLinkVideoFrameAncillaryPackets * This);
        
        END_INTERFACE
    } IDeckLinkVideoFrameAncillaryPacketsVtbl;

    interface IDeckLinkVideoFrameAncillaryPackets
    {
        CONST_VTBL struct IDeckLinkVideoFrameAncillaryPacketsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrameAncillaryPackets_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrameAncillaryPackets_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrameAncillaryPackets_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrameAncillaryPackets_GetPacketIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetPacketIterator(This,iterator) ) 

#define IDeckLinkVideoFrameAncillaryPackets_GetFirstPacketByID(This,DID,SDID,packet)	\
    ( (This)->lpVtbl -> GetFirstPacketByID(This,DID,SDID,packet) ) 

#define IDeckLinkVideoFrameAncillaryPackets_AttachPacket(This,packet)	\
    ( (This)->lpVtbl -> AttachPacket(This,packet) ) 

#define IDeckLinkVideoFrameAncillaryPackets_DetachPacket(This,packet)	\
    ( (This)->lpVtbl -> DetachPacket(This,packet) ) 

#define IDeckLinkVideoFrameAncillaryPackets_DetachAllPackets(This)	\
    ( (This)->lpVtbl -> DetachAllPackets(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrameAncillaryPackets_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrameAncillary_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrameAncillary_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrameAncillary */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrameAncillary;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("732E723C-D1A4-4E29-9E8E-4A88797A0004")
    IDeckLinkVideoFrameAncillary : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBufferForVerticalBlankingLine( 
            /* [in] */ unsigned int lineNumber,
            /* [out] */ void **buffer) = 0;
        
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void) = 0;
        
        virtual BMDDisplayMode STDMETHODCALLTYPE GetDisplayMode( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameAncillaryVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrameAncillary * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrameAncillary * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrameAncillary * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillary, GetBufferForVerticalBlankingLine)
        HRESULT ( STDMETHODCALLTYPE *GetBufferForVerticalBlankingLine )( 
            IDeckLinkVideoFrameAncillary * This,
            /* [in] */ unsigned int lineNumber,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillary, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoFrameAncillary * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameAncillary, GetDisplayMode)
        BMDDisplayMode ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkVideoFrameAncillary * This);
        
        END_INTERFACE
    } IDeckLinkVideoFrameAncillaryVtbl;

    interface IDeckLinkVideoFrameAncillary
    {
        CONST_VTBL struct IDeckLinkVideoFrameAncillaryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrameAncillary_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrameAncillary_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrameAncillary_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrameAncillary_GetBufferForVerticalBlankingLine(This,lineNumber,buffer)	\
    ( (This)->lpVtbl -> GetBufferForVerticalBlankingLine(This,lineNumber,buffer) ) 

#define IDeckLinkVideoFrameAncillary_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoFrameAncillary_GetDisplayMode(This)	\
    ( (This)->lpVtbl -> GetDisplayMode(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrameAncillary_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderPacket_INTERFACE_DEFINED__
#define __IDeckLinkEncoderPacket_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderPacket */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B693F36C-316E-4AF1-B6C2-F389A4BCA620")
    IDeckLinkEncoderPacket : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual long STDMETHODCALLTYPE GetSize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamTime( 
            /* [out] */ BMDTimeValue *frameTime,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual BMDPacketType STDMETHODCALLTYPE GetPacketType( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkEncoderPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetSize)
        long ( STDMETHODCALLTYPE *GetSize )( 
            IDeckLinkEncoderPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkEncoderPacket * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetPacketType)
        BMDPacketType ( STDMETHODCALLTYPE *GetPacketType )( 
            IDeckLinkEncoderPacket * This);
        
        END_INTERFACE
    } IDeckLinkEncoderPacketVtbl;

    interface IDeckLinkEncoderPacket
    {
        CONST_VTBL struct IDeckLinkEncoderPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkEncoderPacket_GetSize(This)	\
    ( (This)->lpVtbl -> GetSize(This) ) 

#define IDeckLinkEncoderPacket_GetStreamTime(This,frameTime,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,timeScale) ) 

#define IDeckLinkEncoderPacket_GetPacketType(This)	\
    ( (This)->lpVtbl -> GetPacketType(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderPacket_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderVideoPacket_INTERFACE_DEFINED__
#define __IDeckLinkEncoderVideoPacket_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderVideoPacket */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderVideoPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4E7FD944-E8C7-4EAC-B8C0-7B77F80F5AE0")
    IDeckLinkEncoderVideoPacket : public IDeckLinkEncoderPacket
    {
    public:
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceTimestamp( 
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecode( 
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderVideoPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderVideoPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderVideoPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderVideoPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkEncoderVideoPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetSize)
        long ( STDMETHODCALLTYPE *GetSize )( 
            IDeckLinkEncoderVideoPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkEncoderVideoPacket * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetPacketType)
        BMDPacketType ( STDMETHODCALLTYPE *GetPacketType )( 
            IDeckLinkEncoderVideoPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderVideoPacket, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkEncoderVideoPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderVideoPacket, GetHardwareReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceTimestamp )( 
            IDeckLinkEncoderVideoPacket * This,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderVideoPacket, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkEncoderVideoPacket * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        END_INTERFACE
    } IDeckLinkEncoderVideoPacketVtbl;

    interface IDeckLinkEncoderVideoPacket
    {
        CONST_VTBL struct IDeckLinkEncoderVideoPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderVideoPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderVideoPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderVideoPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderVideoPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkEncoderVideoPacket_GetSize(This)	\
    ( (This)->lpVtbl -> GetSize(This) ) 

#define IDeckLinkEncoderVideoPacket_GetStreamTime(This,frameTime,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,timeScale) ) 

#define IDeckLinkEncoderVideoPacket_GetPacketType(This)	\
    ( (This)->lpVtbl -> GetPacketType(This) ) 


#define IDeckLinkEncoderVideoPacket_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkEncoderVideoPacket_GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration)	\
    ( (This)->lpVtbl -> GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration) ) 

#define IDeckLinkEncoderVideoPacket_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderVideoPacket_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderAudioPacket_INTERFACE_DEFINED__
#define __IDeckLinkEncoderAudioPacket_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderAudioPacket */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderAudioPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49E8EDC8-693B-4E14-8EF6-12C658F5A07A")
    IDeckLinkEncoderAudioPacket : public IDeckLinkEncoderPacket
    {
    public:
        virtual BMDAudioFormat STDMETHODCALLTYPE GetAudioFormat( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderAudioPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderAudioPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkEncoderAudioPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetSize)
        long ( STDMETHODCALLTYPE *GetSize )( 
            IDeckLinkEncoderAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkEncoderAudioPacket * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetPacketType)
        BMDPacketType ( STDMETHODCALLTYPE *GetPacketType )( 
            IDeckLinkEncoderAudioPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderAudioPacket, GetAudioFormat)
        BMDAudioFormat ( STDMETHODCALLTYPE *GetAudioFormat )( 
            IDeckLinkEncoderAudioPacket * This);
        
        END_INTERFACE
    } IDeckLinkEncoderAudioPacketVtbl;

    interface IDeckLinkEncoderAudioPacket
    {
        CONST_VTBL struct IDeckLinkEncoderAudioPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderAudioPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderAudioPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderAudioPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderAudioPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkEncoderAudioPacket_GetSize(This)	\
    ( (This)->lpVtbl -> GetSize(This) ) 

#define IDeckLinkEncoderAudioPacket_GetStreamTime(This,frameTime,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,timeScale) ) 

#define IDeckLinkEncoderAudioPacket_GetPacketType(This)	\
    ( (This)->lpVtbl -> GetPacketType(This) ) 


#define IDeckLinkEncoderAudioPacket_GetAudioFormat(This)	\
    ( (This)->lpVtbl -> GetAudioFormat(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderAudioPacket_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkH265NALPacket_INTERFACE_DEFINED__
#define __IDeckLinkH265NALPacket_INTERFACE_DEFINED__

/* interface IDeckLinkH265NALPacket */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkH265NALPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("639C8E0B-68D5-4BDE-A6D4-95F3AEAFF2E7")
    IDeckLinkH265NALPacket : public IDeckLinkEncoderVideoPacket
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetUnitType( 
            /* [out] */ unsigned char *unitType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytesNoPrefix( 
            /* [out] */ void **buffer) = 0;
        
        virtual long STDMETHODCALLTYPE GetSizeNoPrefix( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkH265NALPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkH265NALPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkH265NALPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkH265NALPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkH265NALPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetSize)
        long ( STDMETHODCALLTYPE *GetSize )( 
            IDeckLinkH265NALPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkH265NALPacket * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderPacket, GetPacketType)
        BMDPacketType ( STDMETHODCALLTYPE *GetPacketType )( 
            IDeckLinkH265NALPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderVideoPacket, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkH265NALPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderVideoPacket, GetHardwareReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceTimestamp )( 
            IDeckLinkH265NALPacket * This,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderVideoPacket, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkH265NALPacket * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkH265NALPacket, GetUnitType)
        HRESULT ( STDMETHODCALLTYPE *GetUnitType )( 
            IDeckLinkH265NALPacket * This,
            /* [out] */ unsigned char *unitType);
        
        DECLSPEC_XFGVIRT(IDeckLinkH265NALPacket, GetBytesNoPrefix)
        HRESULT ( STDMETHODCALLTYPE *GetBytesNoPrefix )( 
            IDeckLinkH265NALPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkH265NALPacket, GetSizeNoPrefix)
        long ( STDMETHODCALLTYPE *GetSizeNoPrefix )( 
            IDeckLinkH265NALPacket * This);
        
        END_INTERFACE
    } IDeckLinkH265NALPacketVtbl;

    interface IDeckLinkH265NALPacket
    {
        CONST_VTBL struct IDeckLinkH265NALPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkH265NALPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkH265NALPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkH265NALPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkH265NALPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkH265NALPacket_GetSize(This)	\
    ( (This)->lpVtbl -> GetSize(This) ) 

#define IDeckLinkH265NALPacket_GetStreamTime(This,frameTime,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,timeScale) ) 

#define IDeckLinkH265NALPacket_GetPacketType(This)	\
    ( (This)->lpVtbl -> GetPacketType(This) ) 


#define IDeckLinkH265NALPacket_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkH265NALPacket_GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration)	\
    ( (This)->lpVtbl -> GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration) ) 

#define IDeckLinkH265NALPacket_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 


#define IDeckLinkH265NALPacket_GetUnitType(This,unitType)	\
    ( (This)->lpVtbl -> GetUnitType(This,unitType) ) 

#define IDeckLinkH265NALPacket_GetBytesNoPrefix(This,buffer)	\
    ( (This)->lpVtbl -> GetBytesNoPrefix(This,buffer) ) 

#define IDeckLinkH265NALPacket_GetSizeNoPrefix(This)	\
    ( (This)->lpVtbl -> GetSizeNoPrefix(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkH265NALPacket_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAudioInputPacket_INTERFACE_DEFINED__
#define __IDeckLinkAudioInputPacket_INTERFACE_DEFINED__

/* interface IDeckLinkAudioInputPacket */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAudioInputPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E43D5870-2894-11DE-8C30-0800200C9A66")
    IDeckLinkAudioInputPacket : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetSampleFrameCount( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPacketTime( 
            /* [out] */ BMDTimeValue *packetTime,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkAudioInputPacketVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAudioInputPacket * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAudioInputPacket * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAudioInputPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAudioInputPacket, GetSampleFrameCount)
        long ( STDMETHODCALLTYPE *GetSampleFrameCount )( 
            IDeckLinkAudioInputPacket * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAudioInputPacket, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkAudioInputPacket * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkAudioInputPacket, GetPacketTime)
        HRESULT ( STDMETHODCALLTYPE *GetPacketTime )( 
            IDeckLinkAudioInputPacket * This,
            /* [out] */ BMDTimeValue *packetTime,
            /* [in] */ BMDTimeScale timeScale);
        
        END_INTERFACE
    } IDeckLinkAudioInputPacketVtbl;

    interface IDeckLinkAudioInputPacket
    {
        CONST_VTBL struct IDeckLinkAudioInputPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAudioInputPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAudioInputPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAudioInputPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAudioInputPacket_GetSampleFrameCount(This)	\
    ( (This)->lpVtbl -> GetSampleFrameCount(This) ) 

#define IDeckLinkAudioInputPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkAudioInputPacket_GetPacketTime(This,packetTime,timeScale)	\
    ( (This)->lpVtbl -> GetPacketTime(This,packetTime,timeScale) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAudioInputPacket_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkScreenPreviewCallback_INTERFACE_DEFINED__
#define __IDeckLinkScreenPreviewCallback_INTERFACE_DEFINED__

/* interface IDeckLinkScreenPreviewCallback */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkScreenPreviewCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D4FA2345-9FBA-4497-95C3-C0C3CED3CDA8")
    IDeckLinkScreenPreviewCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DrawFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkScreenPreviewCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkScreenPreviewCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkScreenPreviewCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkScreenPreviewCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkScreenPreviewCallback, DrawFrame)
        HRESULT ( STDMETHODCALLTYPE *DrawFrame )( 
            IDeckLinkScreenPreviewCallback * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        END_INTERFACE
    } IDeckLinkScreenPreviewCallbackVtbl;

    interface IDeckLinkScreenPreviewCallback
    {
        CONST_VTBL struct IDeckLinkScreenPreviewCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkScreenPreviewCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkScreenPreviewCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkScreenPreviewCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkScreenPreviewCallback_DrawFrame(This,theFrame)	\
    ( (This)->lpVtbl -> DrawFrame(This,theFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkScreenPreviewCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkGLScreenPreviewHelper_INTERFACE_DEFINED__
#define __IDeckLinkGLScreenPreviewHelper_INTERFACE_DEFINED__

/* interface IDeckLinkGLScreenPreviewHelper */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkGLScreenPreviewHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CEB778E2-C202-4EC8-9085-0CD285CC5522")
    IDeckLinkGLScreenPreviewHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitializeGL( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PaintGL( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set3DPreviewFormat( 
            /* [in] */ BMD3DPreviewFormat previewFormat) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkGLScreenPreviewHelperVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkGLScreenPreviewHelper * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper, InitializeGL)
        HRESULT ( STDMETHODCALLTYPE *InitializeGL )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper, PaintGL)
        HRESULT ( STDMETHODCALLTYPE *PaintGL )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper, SetFrame)
        HRESULT ( STDMETHODCALLTYPE *SetFrame )( 
            IDeckLinkGLScreenPreviewHelper * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper, Set3DPreviewFormat)
        HRESULT ( STDMETHODCALLTYPE *Set3DPreviewFormat )( 
            IDeckLinkGLScreenPreviewHelper * This,
            /* [in] */ BMD3DPreviewFormat previewFormat);
        
        END_INTERFACE
    } IDeckLinkGLScreenPreviewHelperVtbl;

    interface IDeckLinkGLScreenPreviewHelper
    {
        CONST_VTBL struct IDeckLinkGLScreenPreviewHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkGLScreenPreviewHelper_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkGLScreenPreviewHelper_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkGLScreenPreviewHelper_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkGLScreenPreviewHelper_InitializeGL(This)	\
    ( (This)->lpVtbl -> InitializeGL(This) ) 

#define IDeckLinkGLScreenPreviewHelper_PaintGL(This)	\
    ( (This)->lpVtbl -> PaintGL(This) ) 

#define IDeckLinkGLScreenPreviewHelper_SetFrame(This,theFrame)	\
    ( (This)->lpVtbl -> SetFrame(This,theFrame) ) 

#define IDeckLinkGLScreenPreviewHelper_Set3DPreviewFormat(This,previewFormat)	\
    ( (This)->lpVtbl -> Set3DPreviewFormat(This,previewFormat) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkGLScreenPreviewHelper_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDX9ScreenPreviewHelper_INTERFACE_DEFINED__
#define __IDeckLinkDX9ScreenPreviewHelper_INTERFACE_DEFINED__

/* interface IDeckLinkDX9ScreenPreviewHelper */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDX9ScreenPreviewHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F2DD78CA-2921-4AC2-B5BC-BFDCC2035A1F")
    IDeckLinkDX9ScreenPreviewHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ void *device) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Render( 
            /* [in] */ RECT *rc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set3DPreviewFormat( 
            /* [in] */ BMD3DPreviewFormat previewFormat) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDX9ScreenPreviewHelperVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDX9ScreenPreviewHelper * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDX9ScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDX9ScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper, Initialize)
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDeckLinkDX9ScreenPreviewHelper * This,
            /* [in] */ void *device);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper, Render)
        HRESULT ( STDMETHODCALLTYPE *Render )( 
            IDeckLinkDX9ScreenPreviewHelper * This,
            /* [in] */ RECT *rc);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper, SetFrame)
        HRESULT ( STDMETHODCALLTYPE *SetFrame )( 
            IDeckLinkDX9ScreenPreviewHelper * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper, Set3DPreviewFormat)
        HRESULT ( STDMETHODCALLTYPE *Set3DPreviewFormat )( 
            IDeckLinkDX9ScreenPreviewHelper * This,
            /* [in] */ BMD3DPreviewFormat previewFormat);
        
        END_INTERFACE
    } IDeckLinkDX9ScreenPreviewHelperVtbl;

    interface IDeckLinkDX9ScreenPreviewHelper
    {
        CONST_VTBL struct IDeckLinkDX9ScreenPreviewHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDX9ScreenPreviewHelper_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDX9ScreenPreviewHelper_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDX9ScreenPreviewHelper_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDX9ScreenPreviewHelper_Initialize(This,device)	\
    ( (This)->lpVtbl -> Initialize(This,device) ) 

#define IDeckLinkDX9ScreenPreviewHelper_Render(This,rc)	\
    ( (This)->lpVtbl -> Render(This,rc) ) 

#define IDeckLinkDX9ScreenPreviewHelper_SetFrame(This,theFrame)	\
    ( (This)->lpVtbl -> SetFrame(This,theFrame) ) 

#define IDeckLinkDX9ScreenPreviewHelper_Set3DPreviewFormat(This,previewFormat)	\
    ( (This)->lpVtbl -> Set3DPreviewFormat(This,previewFormat) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDX9ScreenPreviewHelper_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkWPFDX9ScreenPreviewHelper_INTERFACE_DEFINED__
#define __IDeckLinkWPFDX9ScreenPreviewHelper_INTERFACE_DEFINED__

/* interface IDeckLinkWPFDX9ScreenPreviewHelper */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkWPFDX9ScreenPreviewHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C59346CD-9326-4266-AC2D-5C190F5799EE")
    IDeckLinkWPFDX9ScreenPreviewHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Render( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSurfaceSize( 
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set3DPreviewFormat( 
            /* [in] */ BMD3DPreviewFormat previewFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackBuffer( 
            /* [out] */ void **backBuffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkWPFDX9ScreenPreviewHelperVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper, Initialize)
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper, Render)
        HRESULT ( STDMETHODCALLTYPE *Render )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper, SetSurfaceSize)
        HRESULT ( STDMETHODCALLTYPE *SetSurfaceSize )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This,
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper, SetFrame)
        HRESULT ( STDMETHODCALLTYPE *SetFrame )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper, Set3DPreviewFormat)
        HRESULT ( STDMETHODCALLTYPE *Set3DPreviewFormat )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This,
            /* [in] */ BMD3DPreviewFormat previewFormat);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper, GetBackBuffer)
        HRESULT ( STDMETHODCALLTYPE *GetBackBuffer )( 
            IDeckLinkWPFDX9ScreenPreviewHelper * This,
            /* [out] */ void **backBuffer);
        
        END_INTERFACE
    } IDeckLinkWPFDX9ScreenPreviewHelperVtbl;

    interface IDeckLinkWPFDX9ScreenPreviewHelper
    {
        CONST_VTBL struct IDeckLinkWPFDX9ScreenPreviewHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkWPFDX9ScreenPreviewHelper_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkWPFDX9ScreenPreviewHelper_Initialize(This)	\
    ( (This)->lpVtbl -> Initialize(This) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_Render(This)	\
    ( (This)->lpVtbl -> Render(This) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_SetSurfaceSize(This,width,height)	\
    ( (This)->lpVtbl -> SetSurfaceSize(This,width,height) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_SetFrame(This,theFrame)	\
    ( (This)->lpVtbl -> SetFrame(This,theFrame) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_Set3DPreviewFormat(This,previewFormat)	\
    ( (This)->lpVtbl -> Set3DPreviewFormat(This,previewFormat) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_GetBackBuffer(This,backBuffer)	\
    ( (This)->lpVtbl -> GetBackBuffer(This,backBuffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkWPFDX9ScreenPreviewHelper_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkNotificationCallback_INTERFACE_DEFINED__
#define __IDeckLinkNotificationCallback_INTERFACE_DEFINED__

/* interface IDeckLinkNotificationCallback */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkNotificationCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b002a1ec-070d-4288-8289-bd5d36e5ff0d")
    IDeckLinkNotificationCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ BMDNotifications topic,
            /* [in] */ ULONGLONG param1,
            /* [in] */ ULONGLONG param2) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkNotificationCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkNotificationCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkNotificationCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkNotificationCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkNotificationCallback, Notify)
        HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IDeckLinkNotificationCallback * This,
            /* [in] */ BMDNotifications topic,
            /* [in] */ ULONGLONG param1,
            /* [in] */ ULONGLONG param2);
        
        END_INTERFACE
    } IDeckLinkNotificationCallbackVtbl;

    interface IDeckLinkNotificationCallback
    {
        CONST_VTBL struct IDeckLinkNotificationCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkNotificationCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkNotificationCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkNotificationCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkNotificationCallback_Notify(This,topic,param1,param2)	\
    ( (This)->lpVtbl -> Notify(This,topic,param1,param2) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkNotificationCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkNotification_INTERFACE_DEFINED__
#define __IDeckLinkNotification_INTERFACE_DEFINED__

/* interface IDeckLinkNotification */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b85df4c8-bdf5-47c1-8064-28162ebdd4eb")
    IDeckLinkNotification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Subscribe( 
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unsubscribe( 
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkNotificationVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkNotification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkNotification * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkNotification * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkNotification, Subscribe)
        HRESULT ( STDMETHODCALLTYPE *Subscribe )( 
            IDeckLinkNotification * This,
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkNotification, Unsubscribe)
        HRESULT ( STDMETHODCALLTYPE *Unsubscribe )( 
            IDeckLinkNotification * This,
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback);
        
        END_INTERFACE
    } IDeckLinkNotificationVtbl;

    interface IDeckLinkNotification
    {
        CONST_VTBL struct IDeckLinkNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkNotification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkNotification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkNotification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkNotification_Subscribe(This,topic,theCallback)	\
    ( (This)->lpVtbl -> Subscribe(This,topic,theCallback) ) 

#define IDeckLinkNotification_Unsubscribe(This,topic,theCallback)	\
    ( (This)->lpVtbl -> Unsubscribe(This,topic,theCallback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkNotification_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkProfileAttributes_INTERFACE_DEFINED__
#define __IDeckLinkProfileAttributes_INTERFACE_DEFINED__

/* interface IDeckLinkProfileAttributes */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkProfileAttributes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17D4BF8E-4911-473A-80A0-731CF6FF345B")
    IDeckLinkProfileAttributes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkProfileAttributesVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkProfileAttributes * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkProfileAttributes * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkProfileAttributes * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileAttributes, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkProfileAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileAttributes, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkProfileAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileAttributes, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkProfileAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileAttributes, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkProfileAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkProfileAttributesVtbl;

    interface IDeckLinkProfileAttributes
    {
        CONST_VTBL struct IDeckLinkProfileAttributesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkProfileAttributes_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkProfileAttributes_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkProfileAttributes_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkProfileAttributes_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkProfileAttributes_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkProfileAttributes_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkProfileAttributes_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkProfileAttributes_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkProfileIterator_INTERFACE_DEFINED__
#define __IDeckLinkProfileIterator_INTERFACE_DEFINED__

/* interface IDeckLinkProfileIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkProfileIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("29E5A8C0-8BE4-46EB-93AC-31DAAB5B7BF2")
    IDeckLinkProfileIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLinkProfile **profile) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkProfileIteratorVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkProfileIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkProfileIterator * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkProfileIterator * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileIterator, Next)
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkProfileIterator * This,
            /* [out] */ IDeckLinkProfile **profile);
        
        END_INTERFACE
    } IDeckLinkProfileIteratorVtbl;

    interface IDeckLinkProfileIterator
    {
        CONST_VTBL struct IDeckLinkProfileIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkProfileIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkProfileIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkProfileIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkProfileIterator_Next(This,profile)	\
    ( (This)->lpVtbl -> Next(This,profile) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkProfileIterator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkProfile_INTERFACE_DEFINED__
#define __IDeckLinkProfile_INTERFACE_DEFINED__

/* interface IDeckLinkProfile */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkProfile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("16093466-674A-432B-9DA0-1AC2C5A8241C")
    IDeckLinkProfile : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDevice( 
            /* [out] */ IDeckLink **device) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsActive( 
            /* [out] */ BOOL *isActive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActive( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPeers( 
            /* [out] */ IDeckLinkProfileIterator **profileIterator) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkProfileVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkProfile * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkProfile * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkProfile * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfile, GetDevice)
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            IDeckLinkProfile * This,
            /* [out] */ IDeckLink **device);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfile, IsActive)
        HRESULT ( STDMETHODCALLTYPE *IsActive )( 
            IDeckLinkProfile * This,
            /* [out] */ BOOL *isActive);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfile, SetActive)
        HRESULT ( STDMETHODCALLTYPE *SetActive )( 
            IDeckLinkProfile * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfile, GetPeers)
        HRESULT ( STDMETHODCALLTYPE *GetPeers )( 
            IDeckLinkProfile * This,
            /* [out] */ IDeckLinkProfileIterator **profileIterator);
        
        END_INTERFACE
    } IDeckLinkProfileVtbl;

    interface IDeckLinkProfile
    {
        CONST_VTBL struct IDeckLinkProfileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkProfile_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkProfile_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkProfile_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkProfile_GetDevice(This,device)	\
    ( (This)->lpVtbl -> GetDevice(This,device) ) 

#define IDeckLinkProfile_IsActive(This,isActive)	\
    ( (This)->lpVtbl -> IsActive(This,isActive) ) 

#define IDeckLinkProfile_SetActive(This)	\
    ( (This)->lpVtbl -> SetActive(This) ) 

#define IDeckLinkProfile_GetPeers(This,profileIterator)	\
    ( (This)->lpVtbl -> GetPeers(This,profileIterator) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkProfile_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkProfileCallback_INTERFACE_DEFINED__
#define __IDeckLinkProfileCallback_INTERFACE_DEFINED__

/* interface IDeckLinkProfileCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkProfileCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A4F9341E-97AA-4E04-8935-15F809898CEA")
    IDeckLinkProfileCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ProfileChanging( 
            /* [in] */ IDeckLinkProfile *profileToBeActivated,
            /* [in] */ BOOL streamsWillBeForcedToStop) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProfileActivated( 
            /* [in] */ IDeckLinkProfile *activatedProfile) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkProfileCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkProfileCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkProfileCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkProfileCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileCallback, ProfileChanging)
        HRESULT ( STDMETHODCALLTYPE *ProfileChanging )( 
            IDeckLinkProfileCallback * This,
            /* [in] */ IDeckLinkProfile *profileToBeActivated,
            /* [in] */ BOOL streamsWillBeForcedToStop);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileCallback, ProfileActivated)
        HRESULT ( STDMETHODCALLTYPE *ProfileActivated )( 
            IDeckLinkProfileCallback * This,
            /* [in] */ IDeckLinkProfile *activatedProfile);
        
        END_INTERFACE
    } IDeckLinkProfileCallbackVtbl;

    interface IDeckLinkProfileCallback
    {
        CONST_VTBL struct IDeckLinkProfileCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkProfileCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkProfileCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkProfileCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkProfileCallback_ProfileChanging(This,profileToBeActivated,streamsWillBeForcedToStop)	\
    ( (This)->lpVtbl -> ProfileChanging(This,profileToBeActivated,streamsWillBeForcedToStop) ) 

#define IDeckLinkProfileCallback_ProfileActivated(This,activatedProfile)	\
    ( (This)->lpVtbl -> ProfileActivated(This,activatedProfile) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkProfileCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkProfileManager_INTERFACE_DEFINED__
#define __IDeckLinkProfileManager_INTERFACE_DEFINED__

/* interface IDeckLinkProfileManager */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkProfileManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30D41429-3998-4B6D-84F8-78C94A797C6E")
    IDeckLinkProfileManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProfiles( 
            /* [out] */ IDeckLinkProfileIterator **profileIterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProfile( 
            /* [in] */ BMDProfileID profileID,
            /* [out] */ IDeckLinkProfile **profile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkProfileCallback *callback) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkProfileManagerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkProfileManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkProfileManager * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkProfileManager * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileManager, GetProfiles)
        HRESULT ( STDMETHODCALLTYPE *GetProfiles )( 
            IDeckLinkProfileManager * This,
            /* [out] */ IDeckLinkProfileIterator **profileIterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileManager, GetProfile)
        HRESULT ( STDMETHODCALLTYPE *GetProfile )( 
            IDeckLinkProfileManager * This,
            /* [in] */ BMDProfileID profileID,
            /* [out] */ IDeckLinkProfile **profile);
        
        DECLSPEC_XFGVIRT(IDeckLinkProfileManager, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkProfileManager * This,
            /* [in] */ IDeckLinkProfileCallback *callback);
        
        END_INTERFACE
    } IDeckLinkProfileManagerVtbl;

    interface IDeckLinkProfileManager
    {
        CONST_VTBL struct IDeckLinkProfileManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkProfileManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkProfileManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkProfileManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkProfileManager_GetProfiles(This,profileIterator)	\
    ( (This)->lpVtbl -> GetProfiles(This,profileIterator) ) 

#define IDeckLinkProfileManager_GetProfile(This,profileID,profile)	\
    ( (This)->lpVtbl -> GetProfile(This,profileID,profile) ) 

#define IDeckLinkProfileManager_SetCallback(This,callback)	\
    ( (This)->lpVtbl -> SetCallback(This,callback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkProfileManager_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkStatus_INTERFACE_DEFINED__
#define __IDeckLinkStatus_INTERFACE_DEFINED__

/* interface IDeckLinkStatus */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5F558200-4028-49BC-BEAC-DB3FA4A96E46")
    IDeckLinkStatus : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ void *buffer,
            /* [out][in] */ unsigned int *bufferSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkStatusVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkStatus * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkStatus * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkStatus * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkStatus, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkStatus * This,
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkStatus, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkStatus * This,
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkStatus, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkStatus * This,
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkStatus, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkStatus * This,
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkStatus, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkStatus * This,
            /* [in] */ BMDDeckLinkStatusID statusID,
            /* [out] */ void *buffer,
            /* [out][in] */ unsigned int *bufferSize);
        
        END_INTERFACE
    } IDeckLinkStatusVtbl;

    interface IDeckLinkStatus
    {
        CONST_VTBL struct IDeckLinkStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkStatus_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkStatus_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkStatus_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkStatus_GetFlag(This,statusID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,statusID,value) ) 

#define IDeckLinkStatus_GetInt(This,statusID,value)	\
    ( (This)->lpVtbl -> GetInt(This,statusID,value) ) 

#define IDeckLinkStatus_GetFloat(This,statusID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,statusID,value) ) 

#define IDeckLinkStatus_GetString(This,statusID,value)	\
    ( (This)->lpVtbl -> GetString(This,statusID,value) ) 

#define IDeckLinkStatus_GetBytes(This,statusID,buffer,bufferSize)	\
    ( (This)->lpVtbl -> GetBytes(This,statusID,buffer,bufferSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkStatus_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkKeyer_INTERFACE_DEFINED__
#define __IDeckLinkKeyer_INTERFACE_DEFINED__

/* interface IDeckLinkKeyer */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkKeyer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89AFCAF5-65F8-421E-98F7-96FE5F5BFBA3")
    IDeckLinkKeyer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Enable( 
            /* [in] */ BOOL isExternal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLevel( 
            /* [in] */ unsigned char level) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RampUp( 
            /* [in] */ unsigned int numberOfFrames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RampDown( 
            /* [in] */ unsigned int numberOfFrames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disable( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkKeyerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkKeyer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkKeyer * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkKeyer * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkKeyer, Enable)
        HRESULT ( STDMETHODCALLTYPE *Enable )( 
            IDeckLinkKeyer * This,
            /* [in] */ BOOL isExternal);
        
        DECLSPEC_XFGVIRT(IDeckLinkKeyer, SetLevel)
        HRESULT ( STDMETHODCALLTYPE *SetLevel )( 
            IDeckLinkKeyer * This,
            /* [in] */ unsigned char level);
        
        DECLSPEC_XFGVIRT(IDeckLinkKeyer, RampUp)
        HRESULT ( STDMETHODCALLTYPE *RampUp )( 
            IDeckLinkKeyer * This,
            /* [in] */ unsigned int numberOfFrames);
        
        DECLSPEC_XFGVIRT(IDeckLinkKeyer, RampDown)
        HRESULT ( STDMETHODCALLTYPE *RampDown )( 
            IDeckLinkKeyer * This,
            /* [in] */ unsigned int numberOfFrames);
        
        DECLSPEC_XFGVIRT(IDeckLinkKeyer, Disable)
        HRESULT ( STDMETHODCALLTYPE *Disable )( 
            IDeckLinkKeyer * This);
        
        END_INTERFACE
    } IDeckLinkKeyerVtbl;

    interface IDeckLinkKeyer
    {
        CONST_VTBL struct IDeckLinkKeyerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkKeyer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkKeyer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkKeyer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkKeyer_Enable(This,isExternal)	\
    ( (This)->lpVtbl -> Enable(This,isExternal) ) 

#define IDeckLinkKeyer_SetLevel(This,level)	\
    ( (This)->lpVtbl -> SetLevel(This,level) ) 

#define IDeckLinkKeyer_RampUp(This,numberOfFrames)	\
    ( (This)->lpVtbl -> RampUp(This,numberOfFrames) ) 

#define IDeckLinkKeyer_RampDown(This,numberOfFrames)	\
    ( (This)->lpVtbl -> RampDown(This,numberOfFrames) ) 

#define IDeckLinkKeyer_Disable(This)	\
    ( (This)->lpVtbl -> Disable(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkKeyer_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoConversion_INTERFACE_DEFINED__
#define __IDeckLinkVideoConversion_INTERFACE_DEFINED__

/* interface IDeckLinkVideoConversion */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoConversion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A48755D9-8BD5-4727-A1E9-069FDEDBA6E9")
    IDeckLinkVideoConversion : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConvertFrame( 
            /* [in] */ IDeckLinkVideoFrame *srcFrame,
            /* [in] */ IDeckLinkVideoFrame *dstFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConvertNewFrame( 
            /* [in] */ IDeckLinkVideoFrame *srcFrame,
            /* [in] */ BMDPixelFormat dstPixelFormat,
            /* [in] */ BMDColorspace dstColorspace,
            /* [in] */ IDeckLinkVideoBuffer *dstBuffer,
            /* [out] */ IDeckLinkVideoFrame **dstFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoConversionVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoConversion * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoConversion * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoConversion * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoConversion, ConvertFrame)
        HRESULT ( STDMETHODCALLTYPE *ConvertFrame )( 
            IDeckLinkVideoConversion * This,
            /* [in] */ IDeckLinkVideoFrame *srcFrame,
            /* [in] */ IDeckLinkVideoFrame *dstFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoConversion, ConvertNewFrame)
        HRESULT ( STDMETHODCALLTYPE *ConvertNewFrame )( 
            IDeckLinkVideoConversion * This,
            /* [in] */ IDeckLinkVideoFrame *srcFrame,
            /* [in] */ BMDPixelFormat dstPixelFormat,
            /* [in] */ BMDColorspace dstColorspace,
            /* [in] */ IDeckLinkVideoBuffer *dstBuffer,
            /* [out] */ IDeckLinkVideoFrame **dstFrame);
        
        END_INTERFACE
    } IDeckLinkVideoConversionVtbl;

    interface IDeckLinkVideoConversion
    {
        CONST_VTBL struct IDeckLinkVideoConversionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoConversion_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoConversion_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoConversion_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoConversion_ConvertFrame(This,srcFrame,dstFrame)	\
    ( (This)->lpVtbl -> ConvertFrame(This,srcFrame,dstFrame) ) 

#define IDeckLinkVideoConversion_ConvertNewFrame(This,srcFrame,dstPixelFormat,dstColorspace,dstBuffer,dstFrame)	\
    ( (This)->lpVtbl -> ConvertNewFrame(This,srcFrame,dstPixelFormat,dstColorspace,dstBuffer,dstFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoConversion_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDeviceNotificationCallback_INTERFACE_DEFINED__
#define __IDeckLinkDeviceNotificationCallback_INTERFACE_DEFINED__

/* interface IDeckLinkDeviceNotificationCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDeviceNotificationCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4997053B-0ADF-4CC8-AC70-7A50C4BE728F")
    IDeckLinkDeviceNotificationCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DeckLinkDeviceArrived( 
            /* [in] */ IDeckLink *deckLinkDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeckLinkDeviceRemoved( 
            /* [in] */ IDeckLink *deckLinkDevice) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDeviceNotificationCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDeviceNotificationCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDeviceNotificationCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDeviceNotificationCallback * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeviceNotificationCallback, DeckLinkDeviceArrived)
        HRESULT ( STDMETHODCALLTYPE *DeckLinkDeviceArrived )( 
            IDeckLinkDeviceNotificationCallback * This,
            /* [in] */ IDeckLink *deckLinkDevice);
        
        DECLSPEC_XFGVIRT(IDeckLinkDeviceNotificationCallback, DeckLinkDeviceRemoved)
        HRESULT ( STDMETHODCALLTYPE *DeckLinkDeviceRemoved )( 
            IDeckLinkDeviceNotificationCallback * This,
            /* [in] */ IDeckLink *deckLinkDevice);
        
        END_INTERFACE
    } IDeckLinkDeviceNotificationCallbackVtbl;

    interface IDeckLinkDeviceNotificationCallback
    {
        CONST_VTBL struct IDeckLinkDeviceNotificationCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDeviceNotificationCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDeviceNotificationCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDeviceNotificationCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDeviceNotificationCallback_DeckLinkDeviceArrived(This,deckLinkDevice)	\
    ( (This)->lpVtbl -> DeckLinkDeviceArrived(This,deckLinkDevice) ) 

#define IDeckLinkDeviceNotificationCallback_DeckLinkDeviceRemoved(This,deckLinkDevice)	\
    ( (This)->lpVtbl -> DeckLinkDeviceRemoved(This,deckLinkDevice) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDeviceNotificationCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDiscovery_INTERFACE_DEFINED__
#define __IDeckLinkDiscovery_INTERFACE_DEFINED__

/* interface IDeckLinkDiscovery */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDiscovery;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CDBF631C-BC76-45FA-B44D-C55059BC6101")
    IDeckLinkDiscovery : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InstallDeviceNotifications( 
            /* [in] */ IDeckLinkDeviceNotificationCallback *deviceNotificationCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UninstallDeviceNotifications( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDiscoveryVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDiscovery * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDiscovery * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDiscovery * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDiscovery, InstallDeviceNotifications)
        HRESULT ( STDMETHODCALLTYPE *InstallDeviceNotifications )( 
            IDeckLinkDiscovery * This,
            /* [in] */ IDeckLinkDeviceNotificationCallback *deviceNotificationCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkDiscovery, UninstallDeviceNotifications)
        HRESULT ( STDMETHODCALLTYPE *UninstallDeviceNotifications )( 
            IDeckLinkDiscovery * This);
        
        END_INTERFACE
    } IDeckLinkDiscoveryVtbl;

    interface IDeckLinkDiscovery
    {
        CONST_VTBL struct IDeckLinkDiscoveryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDiscovery_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDiscovery_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDiscovery_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDiscovery_InstallDeviceNotifications(This,deviceNotificationCallback)	\
    ( (This)->lpVtbl -> InstallDeviceNotifications(This,deviceNotificationCallback) ) 

#define IDeckLinkDiscovery_UninstallDeviceNotifications(This)	\
    ( (This)->lpVtbl -> UninstallDeviceNotifications(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDiscovery_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CDeckLinkIterator;

#ifdef __cplusplus

class DECLSPEC_UUID("BA6C6F44-6DA5-4DCE-94AA-EE2D1372A676")
CDeckLinkIterator;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkAPIInformation;

#ifdef __cplusplus

class DECLSPEC_UUID("263CA19F-ED09-482E-9F9D-84005783A237")
CDeckLinkAPIInformation;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkGLScreenPreviewHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("1E332DAE-0D04-49EB-B8A1-B6E00B2B6BD0")
CDeckLinkGLScreenPreviewHelper;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkGL3ScreenPreviewHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("166804E4-15EF-4BFD-B623-B5BA921667C5")
CDeckLinkGL3ScreenPreviewHelper;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkDX9ScreenPreviewHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("0EB111ED-ADA6-43A6-8B16-CA5D27EEA15E")
CDeckLinkDX9ScreenPreviewHelper;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkWPFDX9ScreenPreviewHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("5E64496D-4BB2-45D5-9B63-BF1B463B18AF")
CDeckLinkWPFDX9ScreenPreviewHelper;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkVideoConversion;

#ifdef __cplusplus

class DECLSPEC_UUID("89BA47BD-1FE2-4D76-9BFE-DE85049C4987")
CDeckLinkVideoConversion;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkDiscovery;

#ifdef __cplusplus

class DECLSPEC_UUID("22FBFC33-8D07-495C-A5BF-DAB5EA9B82DB")
CDeckLinkDiscovery;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkVideoFrameAncillaryPackets;

#ifdef __cplusplus

class DECLSPEC_UUID("F891AD29-D0C2-46E9-A926-4E2D0DD8CFAD")
CDeckLinkVideoFrameAncillaryPackets;
#endif

#ifndef __IDeckLinkVideoOutputCallback_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoOutputCallback_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoOutputCallback_v14_2_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoOutputCallback_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("20AA5225-1958-47CB-820B-80A8D521A6EE")
    IDeckLinkVideoOutputCallback_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoOutputCallback_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoOutputCallback_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoOutputCallback_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoOutputCallback_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoOutputCallback_v14_2_1, ScheduledFrameCompleted)
        HRESULT ( STDMETHODCALLTYPE *ScheduledFrameCompleted )( 
            IDeckLinkVideoOutputCallback_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoOutputCallback_v14_2_1, ScheduledPlaybackHasStopped)
        HRESULT ( STDMETHODCALLTYPE *ScheduledPlaybackHasStopped )( 
            IDeckLinkVideoOutputCallback_v14_2_1 * This);
        
        END_INTERFACE
    } IDeckLinkVideoOutputCallback_v14_2_1Vtbl;

    interface IDeckLinkVideoOutputCallback_v14_2_1
    {
        CONST_VTBL struct IDeckLinkVideoOutputCallback_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoOutputCallback_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoOutputCallback_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoOutputCallback_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoOutputCallback_v14_2_1_ScheduledFrameCompleted(This,completedFrame,result)	\
    ( (This)->lpVtbl -> ScheduledFrameCompleted(This,completedFrame,result) ) 

#define IDeckLinkVideoOutputCallback_v14_2_1_ScheduledPlaybackHasStopped(This)	\
    ( (This)->lpVtbl -> ScheduledPlaybackHasStopped(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoOutputCallback_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInputCallback_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkInputCallback_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkInputCallback_v14_2_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInputCallback_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C6FCE4C9-C4E4-4047-82FB-5D238232A902")
    IDeckLinkInputCallback_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged( 
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived( 
            /* [in] */ IDeckLinkVideoInputFrame_v14_2_1 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputCallback_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInputCallback_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInputCallback_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInputCallback_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInputCallback_v14_2_1, VideoInputFormatChanged)
        HRESULT ( STDMETHODCALLTYPE *VideoInputFormatChanged )( 
            IDeckLinkInputCallback_v14_2_1 * This,
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInputCallback_v14_2_1, VideoInputFrameArrived)
        HRESULT ( STDMETHODCALLTYPE *VideoInputFrameArrived )( 
            IDeckLinkInputCallback_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoInputFrame_v14_2_1 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket);
        
        END_INTERFACE
    } IDeckLinkInputCallback_v14_2_1Vtbl;

    interface IDeckLinkInputCallback_v14_2_1
    {
        CONST_VTBL struct IDeckLinkInputCallback_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInputCallback_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInputCallback_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInputCallback_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInputCallback_v14_2_1_VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags)	\
    ( (This)->lpVtbl -> VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags) ) 

#define IDeckLinkInputCallback_v14_2_1_VideoInputFrameArrived(This,videoFrame,audioPacket)	\
    ( (This)->lpVtbl -> VideoInputFrameArrived(This,videoFrame,audioPacket) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInputCallback_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkMemoryAllocator_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkMemoryAllocator_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkMemoryAllocator_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkMemoryAllocator_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B36EB6E7-9D29-4AA8-92EF-843B87A289E8")
    IDeckLinkMemoryAllocator_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AllocateBuffer( 
            /* [in] */ unsigned int bufferSize,
            /* [out] */ void **allocatedBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer( 
            /* [in] */ void *buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Decommit( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkMemoryAllocator_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkMemoryAllocator_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkMemoryAllocator_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkMemoryAllocator_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkMemoryAllocator_v14_2_1, AllocateBuffer)
        HRESULT ( STDMETHODCALLTYPE *AllocateBuffer )( 
            IDeckLinkMemoryAllocator_v14_2_1 * This,
            /* [in] */ unsigned int bufferSize,
            /* [out] */ void **allocatedBuffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkMemoryAllocator_v14_2_1, ReleaseBuffer)
        HRESULT ( STDMETHODCALLTYPE *ReleaseBuffer )( 
            IDeckLinkMemoryAllocator_v14_2_1 * This,
            /* [in] */ void *buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkMemoryAllocator_v14_2_1, Commit)
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IDeckLinkMemoryAllocator_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkMemoryAllocator_v14_2_1, Decommit)
        HRESULT ( STDMETHODCALLTYPE *Decommit )( 
            IDeckLinkMemoryAllocator_v14_2_1 * This);
        
        END_INTERFACE
    } IDeckLinkMemoryAllocator_v14_2_1Vtbl;

    interface IDeckLinkMemoryAllocator_v14_2_1
    {
        CONST_VTBL struct IDeckLinkMemoryAllocator_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkMemoryAllocator_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkMemoryAllocator_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkMemoryAllocator_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkMemoryAllocator_v14_2_1_AllocateBuffer(This,bufferSize,allocatedBuffer)	\
    ( (This)->lpVtbl -> AllocateBuffer(This,bufferSize,allocatedBuffer) ) 

#define IDeckLinkMemoryAllocator_v14_2_1_ReleaseBuffer(This,buffer)	\
    ( (This)->lpVtbl -> ReleaseBuffer(This,buffer) ) 

#define IDeckLinkMemoryAllocator_v14_2_1_Commit(This)	\
    ( (This)->lpVtbl -> Commit(This) ) 

#define IDeckLinkMemoryAllocator_v14_2_1_Decommit(This)	\
    ( (This)->lpVtbl -> Decommit(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkMemoryAllocator_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkOutput_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkOutput_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkOutput_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkOutput_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BE2D9020-461E-442F-84B7-E949CB953B9D")
    IDeckLinkOutput_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoOutputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoOutput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrame( 
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame_v14_2_1 **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAncillaryData( 
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayVideoFrameSync( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleVideoFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduledFrameCompletionCallback( 
            /* [in] */ IDeckLinkVideoOutputCallback_v14_2_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedVideoFrameCount( 
            /* [out] */ unsigned int *bufferedFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioOutput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAudioSamplesSync( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleAudioSamples( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned int *bufferedSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushBufferedAudioSamples( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioCallback( 
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartScheduledPlayback( 
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopScheduledPlayback( 
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsScheduledPlaybackRunning( 
            /* [out] */ BOOL *active) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScheduledStreamTime( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferenceStatus( 
            /* [out] */ BMDReferenceStatus *referenceStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameCompletionReferenceTimestamp( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkOutput_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkOutput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkOutput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoOutputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, EnableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoOutput )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, DisableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoOutput )( 
            IDeckLinkOutput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, SetVideoOutputFrameMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputFrameMemoryAllocator )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, CreateVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrame )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame_v14_2_1 **outFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, CreateAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *CreateAncillaryData )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, DisplayVideoFrameSync)
        HRESULT ( STDMETHODCALLTYPE *DisplayVideoFrameSync )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, ScheduleVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *ScheduleVideoFrame )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, SetScheduledFrameCompletionCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScheduledFrameCompletionCallback )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoOutputCallback_v14_2_1 *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetBufferedVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedVideoFrameCount )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [out] */ unsigned int *bufferedFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, EnableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioOutput )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, DisableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioOutput )( 
            IDeckLinkOutput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, WriteAudioSamplesSync)
        HRESULT ( STDMETHODCALLTYPE *WriteAudioSamplesSync )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, BeginAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *BeginAudioPreroll )( 
            IDeckLinkOutput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, EndAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *EndAudioPreroll )( 
            IDeckLinkOutput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, ScheduleAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *ScheduleAudioSamples )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetBufferedAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [out] */ unsigned int *bufferedSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, FlushBufferedAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *FlushBufferedAudioSamples )( 
            IDeckLinkOutput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, SetAudioCallback)
        HRESULT ( STDMETHODCALLTYPE *SetAudioCallback )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, StartScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StartScheduledPlayback )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, StopScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StopScheduledPlayback )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, IsScheduledPlaybackRunning)
        HRESULT ( STDMETHODCALLTYPE *IsScheduledPlaybackRunning )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [out] */ BOOL *active);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetScheduledStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetScheduledStreamTime )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetReferenceStatus)
        HRESULT ( STDMETHODCALLTYPE *GetReferenceStatus )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [out] */ BMDReferenceStatus *referenceStatus);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v14_2_1, GetFrameCompletionReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetFrameCompletionReferenceTimestamp )( 
            IDeckLinkOutput_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp);
        
        END_INTERFACE
    } IDeckLinkOutput_v14_2_1Vtbl;

    interface IDeckLinkOutput_v14_2_1
    {
        CONST_VTBL struct IDeckLinkOutput_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkOutput_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkOutput_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkOutput_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkOutput_v14_2_1_DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported) ) 

#define IDeckLinkOutput_v14_2_1_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkOutput_v14_2_1_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkOutput_v14_2_1_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkOutput_v14_2_1_EnableVideoOutput(This,displayMode,flags)	\
    ( (This)->lpVtbl -> EnableVideoOutput(This,displayMode,flags) ) 

#define IDeckLinkOutput_v14_2_1_DisableVideoOutput(This)	\
    ( (This)->lpVtbl -> DisableVideoOutput(This) ) 

#define IDeckLinkOutput_v14_2_1_SetVideoOutputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoOutputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkOutput_v14_2_1_CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_v14_2_1_CreateAncillaryData(This,pixelFormat,outBuffer)	\
    ( (This)->lpVtbl -> CreateAncillaryData(This,pixelFormat,outBuffer) ) 

#define IDeckLinkOutput_v14_2_1_DisplayVideoFrameSync(This,theFrame)	\
    ( (This)->lpVtbl -> DisplayVideoFrameSync(This,theFrame) ) 

#define IDeckLinkOutput_v14_2_1_ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale)	\
    ( (This)->lpVtbl -> ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale) ) 

#define IDeckLinkOutput_v14_2_1_SetScheduledFrameCompletionCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetScheduledFrameCompletionCallback(This,theCallback) ) 

#define IDeckLinkOutput_v14_2_1_GetBufferedVideoFrameCount(This,bufferedFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedVideoFrameCount(This,bufferedFrameCount) ) 

#define IDeckLinkOutput_v14_2_1_EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType)	\
    ( (This)->lpVtbl -> EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType) ) 

#define IDeckLinkOutput_v14_2_1_DisableAudioOutput(This)	\
    ( (This)->lpVtbl -> DisableAudioOutput(This) ) 

#define IDeckLinkOutput_v14_2_1_WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten)	\
    ( (This)->lpVtbl -> WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten) ) 

#define IDeckLinkOutput_v14_2_1_BeginAudioPreroll(This)	\
    ( (This)->lpVtbl -> BeginAudioPreroll(This) ) 

#define IDeckLinkOutput_v14_2_1_EndAudioPreroll(This)	\
    ( (This)->lpVtbl -> EndAudioPreroll(This) ) 

#define IDeckLinkOutput_v14_2_1_ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten)	\
    ( (This)->lpVtbl -> ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten) ) 

#define IDeckLinkOutput_v14_2_1_GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount) ) 

#define IDeckLinkOutput_v14_2_1_FlushBufferedAudioSamples(This)	\
    ( (This)->lpVtbl -> FlushBufferedAudioSamples(This) ) 

#define IDeckLinkOutput_v14_2_1_SetAudioCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetAudioCallback(This,theCallback) ) 

#define IDeckLinkOutput_v14_2_1_StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed)	\
    ( (This)->lpVtbl -> StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed) ) 

#define IDeckLinkOutput_v14_2_1_StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale)	\
    ( (This)->lpVtbl -> StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale) ) 

#define IDeckLinkOutput_v14_2_1_IsScheduledPlaybackRunning(This,active)	\
    ( (This)->lpVtbl -> IsScheduledPlaybackRunning(This,active) ) 

#define IDeckLinkOutput_v14_2_1_GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed)	\
    ( (This)->lpVtbl -> GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed) ) 

#define IDeckLinkOutput_v14_2_1_GetReferenceStatus(This,referenceStatus)	\
    ( (This)->lpVtbl -> GetReferenceStatus(This,referenceStatus) ) 

#define IDeckLinkOutput_v14_2_1_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#define IDeckLinkOutput_v14_2_1_GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp)	\
    ( (This)->lpVtbl -> GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkOutput_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkInput_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkInput_v14_2_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C21CDB6E-F414-46E4-A636-80A566E0ED37")
    IDeckLinkInput_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoInputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoFrameCount( 
            /* [out] */ unsigned int *availableFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoInputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback_v14_2_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInput_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoInputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, GetAvailableVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableVideoFrameCount )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [out] */ unsigned int *availableFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, SetVideoInputFrameMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetVideoInputFrameMemoryAllocator )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ IDeckLinkInputCallback_v14_2_1 *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v14_2_1, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkInput_v14_2_1 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkInput_v14_2_1Vtbl;

    interface IDeckLinkInput_v14_2_1
    {
        CONST_VTBL struct IDeckLinkInput_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_v14_2_1_DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported) ) 

#define IDeckLinkInput_v14_2_1_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkInput_v14_2_1_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_v14_2_1_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkInput_v14_2_1_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_v14_2_1_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_v14_2_1_GetAvailableVideoFrameCount(This,availableFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableVideoFrameCount(This,availableFrameCount) ) 

#define IDeckLinkInput_v14_2_1_SetVideoInputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoInputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkInput_v14_2_1_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_v14_2_1_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_v14_2_1_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkInput_v14_2_1_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_v14_2_1_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_v14_2_1_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_v14_2_1_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkInput_v14_2_1_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkInput_v14_2_1_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderInput_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkEncoderInput_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderInput_v14_2_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderInput_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F222551D-13DF-4FD8-B587-9D4F19EC12C9")
    IDeckLinkEncoderInput_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedCodec,
            /* [in] */ unsigned int requestedCodecProfile,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailablePacketsCount( 
            /* [out] */ unsigned int *availablePacketsCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioFormat audioFormat,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkEncoderInputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderInput_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedCodec,
            /* [in] */ unsigned int requestedCodecProfile,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, GetAvailablePacketsCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailablePacketsCount )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [out] */ unsigned int *availablePacketsCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, SetMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetMemoryAllocator )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ BMDAudioFormat audioFormat,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkEncoderInput_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ IDeckLinkEncoderInputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v14_2_1, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkEncoderInput_v14_2_1 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkEncoderInput_v14_2_1Vtbl;

    interface IDeckLinkEncoderInput_v14_2_1
    {
        CONST_VTBL struct IDeckLinkEncoderInput_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderInput_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderInput_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderInput_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderInput_v14_2_1_DoesSupportVideoMode(This,connection,requestedMode,requestedCodec,requestedCodecProfile,flags,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedCodec,requestedCodecProfile,flags,supported) ) 

#define IDeckLinkEncoderInput_v14_2_1_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkEncoderInput_v14_2_1_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkEncoderInput_v14_2_1_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkEncoderInput_v14_2_1_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkEncoderInput_v14_2_1_GetAvailablePacketsCount(This,availablePacketsCount)	\
    ( (This)->lpVtbl -> GetAvailablePacketsCount(This,availablePacketsCount) ) 

#define IDeckLinkEncoderInput_v14_2_1_SetMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkEncoderInput_v14_2_1_EnableAudioInput(This,audioFormat,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,audioFormat,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkEncoderInput_v14_2_1_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkEncoderInput_v14_2_1_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkEncoderInput_v14_2_1_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkEncoderInput_v14_2_1_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkEncoderInput_v14_2_1_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkEncoderInput_v14_2_1_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkEncoderInput_v14_2_1_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkEncoderInput_v14_2_1_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderInput_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrame_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrame_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrame_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3F716FE0-F023-4111-BE5D-EF4414C05B17")
    IDeckLinkVideoFrame_v14_2_1 : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetWidth( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetHeight( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetRowBytes( void) = 0;
        
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void) = 0;
        
        virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecode( 
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAncillaryData( 
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrame_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrame_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetWidth)
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetHeight)
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetRowBytes)
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetFlags)
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoFrame_v14_2_1 * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkVideoFrame_v14_2_1 * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkVideoFrame_v14_2_1 * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        END_INTERFACE
    } IDeckLinkVideoFrame_v14_2_1Vtbl;

    interface IDeckLinkVideoFrame_v14_2_1
    {
        CONST_VTBL struct IDeckLinkVideoFrame_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrame_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrame_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrame_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrame_v14_2_1_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoFrame_v14_2_1_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoFrame_v14_2_1_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoFrame_v14_2_1_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoFrame_v14_2_1_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoFrame_v14_2_1_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkVideoFrame_v14_2_1_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkVideoFrame_v14_2_1_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrame_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkMutableVideoFrame_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkMutableVideoFrame_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkMutableVideoFrame_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkMutableVideoFrame_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("69E2639F-40DA-4E19-B6F2-20ACE815C390")
    IDeckLinkMutableVideoFrame_v14_2_1 : public IDeckLinkVideoFrame_v14_2_1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlags( 
            /* [in] */ BMDFrameFlags newFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecode( 
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ IDeckLinkTimecode *timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecodeFromComponents( 
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ unsigned char hours,
            /* [in] */ unsigned char minutes,
            /* [in] */ unsigned char seconds,
            /* [in] */ unsigned char frames,
            /* [in] */ BMDTimecodeFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAncillaryData( 
            /* [in] */ IDeckLinkVideoFrameAncillary *ancillary) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecodeUserBits( 
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ BMDTimecodeUserBits userBits) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkMutableVideoFrame_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetWidth)
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetHeight)
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetRowBytes)
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetFlags)
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame_v14_2_1, SetFlags)
        HRESULT ( STDMETHODCALLTYPE *SetFlags )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [in] */ BMDFrameFlags newFlags);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame_v14_2_1, SetTimecode)
        HRESULT ( STDMETHODCALLTYPE *SetTimecode )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ IDeckLinkTimecode *timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame_v14_2_1, SetTimecodeFromComponents)
        HRESULT ( STDMETHODCALLTYPE *SetTimecodeFromComponents )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ unsigned char hours,
            /* [in] */ unsigned char minutes,
            /* [in] */ unsigned char seconds,
            /* [in] */ unsigned char frames,
            /* [in] */ BMDTimecodeFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame_v14_2_1, SetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *SetAncillaryData )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrameAncillary *ancillary);
        
        DECLSPEC_XFGVIRT(IDeckLinkMutableVideoFrame_v14_2_1, SetTimecodeUserBits)
        HRESULT ( STDMETHODCALLTYPE *SetTimecodeUserBits )( 
            IDeckLinkMutableVideoFrame_v14_2_1 * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [in] */ BMDTimecodeUserBits userBits);
        
        END_INTERFACE
    } IDeckLinkMutableVideoFrame_v14_2_1Vtbl;

    interface IDeckLinkMutableVideoFrame_v14_2_1
    {
        CONST_VTBL struct IDeckLinkMutableVideoFrame_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkMutableVideoFrame_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkMutableVideoFrame_v14_2_1_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 


#define IDeckLinkMutableVideoFrame_v14_2_1_SetFlags(This,newFlags)	\
    ( (This)->lpVtbl -> SetFlags(This,newFlags) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_SetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> SetTimecode(This,format,timecode) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_SetTimecodeFromComponents(This,format,hours,minutes,seconds,frames,flags)	\
    ( (This)->lpVtbl -> SetTimecodeFromComponents(This,format,hours,minutes,seconds,frames,flags) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_SetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> SetAncillaryData(This,ancillary) ) 

#define IDeckLinkMutableVideoFrame_v14_2_1_SetTimecodeUserBits(This,format,userBits)	\
    ( (This)->lpVtbl -> SetTimecodeUserBits(This,format,userBits) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkMutableVideoFrame_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrame3DExtensions_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrame3DExtensions_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrame3DExtensions_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrame3DExtensions_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DA0F7E4A-EDC7-48A8-9CDD-2DB51C729CD7")
    IDeckLinkVideoFrame3DExtensions_v14_2_1 : public IUnknown
    {
    public:
        virtual BMDVideo3DPackingFormat STDMETHODCALLTYPE Get3DPackingFormat( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameForRightEye( 
            /* [out] */ IDeckLinkVideoFrame_v14_2_1 **rightEyeFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrame3DExtensions_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrame3DExtensions_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrame3DExtensions_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrame3DExtensions_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame3DExtensions_v14_2_1, Get3DPackingFormat)
        BMDVideo3DPackingFormat ( STDMETHODCALLTYPE *Get3DPackingFormat )( 
            IDeckLinkVideoFrame3DExtensions_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame3DExtensions_v14_2_1, GetFrameForRightEye)
        HRESULT ( STDMETHODCALLTYPE *GetFrameForRightEye )( 
            IDeckLinkVideoFrame3DExtensions_v14_2_1 * This,
            /* [out] */ IDeckLinkVideoFrame_v14_2_1 **rightEyeFrame);
        
        END_INTERFACE
    } IDeckLinkVideoFrame3DExtensions_v14_2_1Vtbl;

    interface IDeckLinkVideoFrame3DExtensions_v14_2_1
    {
        CONST_VTBL struct IDeckLinkVideoFrame3DExtensions_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrame3DExtensions_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrame3DExtensions_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrame3DExtensions_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrame3DExtensions_v14_2_1_Get3DPackingFormat(This)	\
    ( (This)->lpVtbl -> Get3DPackingFormat(This) ) 

#define IDeckLinkVideoFrame3DExtensions_v14_2_1_GetFrameForRightEye(This,rightEyeFrame)	\
    ( (This)->lpVtbl -> GetFrameForRightEye(This,rightEyeFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrame3DExtensions_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoInputFrame_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoInputFrame_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoInputFrame_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("05CFE374-537C-4094-9A57-680525118F44")
    IDeckLinkVideoInputFrame_v14_2_1 : public IDeckLinkVideoFrame_v14_2_1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStreamTime( 
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceTimestamp( 
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoInputFrame_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetWidth)
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetHeight)
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetRowBytes)
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetPixelFormat)
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetFlags)
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetBytes)
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This,
            /* [out] */ void **buffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetTimecode)
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This,
            /* [in] */ BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrame_v14_2_1, GetAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoInputFrame_v14_2_1, GetStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoInputFrame_v14_2_1, GetHardwareReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceTimestamp )( 
            IDeckLinkVideoInputFrame_v14_2_1 * This,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration);
        
        END_INTERFACE
    } IDeckLinkVideoInputFrame_v14_2_1Vtbl;

    interface IDeckLinkVideoInputFrame_v14_2_1
    {
        CONST_VTBL struct IDeckLinkVideoInputFrame_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoInputFrame_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoInputFrame_v14_2_1_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 


#define IDeckLinkVideoInputFrame_v14_2_1_GetStreamTime(This,frameTime,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,frameDuration,timeScale) ) 

#define IDeckLinkVideoInputFrame_v14_2_1_GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration)	\
    ( (This)->lpVtbl -> GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoInputFrame_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkScreenPreviewCallback_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkScreenPreviewCallback_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkScreenPreviewCallback_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkScreenPreviewCallback_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1D3F49A-85FE-4C5D-95C8-0B5D5DCCD438")
    IDeckLinkScreenPreviewCallback_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DrawFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkScreenPreviewCallback_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkScreenPreviewCallback_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkScreenPreviewCallback_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkScreenPreviewCallback_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkScreenPreviewCallback_v14_2_1, DrawFrame)
        HRESULT ( STDMETHODCALLTYPE *DrawFrame )( 
            IDeckLinkScreenPreviewCallback_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame);
        
        END_INTERFACE
    } IDeckLinkScreenPreviewCallback_v14_2_1Vtbl;

    interface IDeckLinkScreenPreviewCallback_v14_2_1
    {
        CONST_VTBL struct IDeckLinkScreenPreviewCallback_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkScreenPreviewCallback_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkScreenPreviewCallback_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkScreenPreviewCallback_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkScreenPreviewCallback_v14_2_1_DrawFrame(This,theFrame)	\
    ( (This)->lpVtbl -> DrawFrame(This,theFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkScreenPreviewCallback_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkGLScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkGLScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkGLScreenPreviewHelper_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkGLScreenPreviewHelper_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("504E2209-CAC7-4C1A-9FB4-C5BB6274D22F")
    IDeckLinkGLScreenPreviewHelper_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitializeGL( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PaintGL( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set3DPreviewFormat( 
            /* [in] */ BMD3DPreviewFormat previewFormat) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkGLScreenPreviewHelper_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkGLScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkGLScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkGLScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper_v14_2_1, InitializeGL)
        HRESULT ( STDMETHODCALLTYPE *InitializeGL )( 
            IDeckLinkGLScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper_v14_2_1, PaintGL)
        HRESULT ( STDMETHODCALLTYPE *PaintGL )( 
            IDeckLinkGLScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper_v14_2_1, SetFrame)
        HRESULT ( STDMETHODCALLTYPE *SetFrame )( 
            IDeckLinkGLScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkGLScreenPreviewHelper_v14_2_1, Set3DPreviewFormat)
        HRESULT ( STDMETHODCALLTYPE *Set3DPreviewFormat )( 
            IDeckLinkGLScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ BMD3DPreviewFormat previewFormat);
        
        END_INTERFACE
    } IDeckLinkGLScreenPreviewHelper_v14_2_1Vtbl;

    interface IDeckLinkGLScreenPreviewHelper_v14_2_1
    {
        CONST_VTBL struct IDeckLinkGLScreenPreviewHelper_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkGLScreenPreviewHelper_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkGLScreenPreviewHelper_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkGLScreenPreviewHelper_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkGLScreenPreviewHelper_v14_2_1_InitializeGL(This)	\
    ( (This)->lpVtbl -> InitializeGL(This) ) 

#define IDeckLinkGLScreenPreviewHelper_v14_2_1_PaintGL(This)	\
    ( (This)->lpVtbl -> PaintGL(This) ) 

#define IDeckLinkGLScreenPreviewHelper_v14_2_1_SetFrame(This,theFrame)	\
    ( (This)->lpVtbl -> SetFrame(This,theFrame) ) 

#define IDeckLinkGLScreenPreviewHelper_v14_2_1_Set3DPreviewFormat(This,previewFormat)	\
    ( (This)->lpVtbl -> Set3DPreviewFormat(This,previewFormat) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkGLScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDX9ScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkDX9ScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkDX9ScreenPreviewHelper_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDX9ScreenPreviewHelper_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2094B522-D1A1-40C0-9AC7-1C012218EF02")
    IDeckLinkDX9ScreenPreviewHelper_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ void *device) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Render( 
            /* [in] */ RECT *rc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set3DPreviewFormat( 
            /* [in] */ BMD3DPreviewFormat previewFormat) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkDX9ScreenPreviewHelper_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDX9ScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDX9ScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper_v14_2_1, Initialize)
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDeckLinkDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ void *device);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper_v14_2_1, Render)
        HRESULT ( STDMETHODCALLTYPE *Render )( 
            IDeckLinkDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ RECT *rc);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper_v14_2_1, SetFrame)
        HRESULT ( STDMETHODCALLTYPE *SetFrame )( 
            IDeckLinkDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkDX9ScreenPreviewHelper_v14_2_1, Set3DPreviewFormat)
        HRESULT ( STDMETHODCALLTYPE *Set3DPreviewFormat )( 
            IDeckLinkDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ BMD3DPreviewFormat previewFormat);
        
        END_INTERFACE
    } IDeckLinkDX9ScreenPreviewHelper_v14_2_1Vtbl;

    interface IDeckLinkDX9ScreenPreviewHelper_v14_2_1
    {
        CONST_VTBL struct IDeckLinkDX9ScreenPreviewHelper_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDX9ScreenPreviewHelper_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDX9ScreenPreviewHelper_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDX9ScreenPreviewHelper_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDX9ScreenPreviewHelper_v14_2_1_Initialize(This,device)	\
    ( (This)->lpVtbl -> Initialize(This,device) ) 

#define IDeckLinkDX9ScreenPreviewHelper_v14_2_1_Render(This,rc)	\
    ( (This)->lpVtbl -> Render(This,rc) ) 

#define IDeckLinkDX9ScreenPreviewHelper_v14_2_1_SetFrame(This,theFrame)	\
    ( (This)->lpVtbl -> SetFrame(This,theFrame) ) 

#define IDeckLinkDX9ScreenPreviewHelper_v14_2_1_Set3DPreviewFormat(This,previewFormat)	\
    ( (This)->lpVtbl -> Set3DPreviewFormat(This,previewFormat) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDX9ScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AD8EC84A-7DDE-11E9-8F9E-2A86E4085A59")
    IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Render( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSurfaceSize( 
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set3DPreviewFormat( 
            /* [in] */ BMD3DPreviewFormat previewFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackBuffer( 
            /* [out] */ void **backBuffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1, Initialize)
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1, Render)
        HRESULT ( STDMETHODCALLTYPE *Render )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1, SetSurfaceSize)
        HRESULT ( STDMETHODCALLTYPE *SetSurfaceSize )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ unsigned int width,
            /* [in] */ unsigned int height);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1, SetFrame)
        HRESULT ( STDMETHODCALLTYPE *SetFrame )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1, Set3DPreviewFormat)
        HRESULT ( STDMETHODCALLTYPE *Set3DPreviewFormat )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [in] */ BMD3DPreviewFormat previewFormat);
        
        DECLSPEC_XFGVIRT(IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1, GetBackBuffer)
        HRESULT ( STDMETHODCALLTYPE *GetBackBuffer )( 
            IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1 * This,
            /* [out] */ void **backBuffer);
        
        END_INTERFACE
    } IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1Vtbl;

    interface IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1
    {
        CONST_VTBL struct IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_Initialize(This)	\
    ( (This)->lpVtbl -> Initialize(This) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_Render(This)	\
    ( (This)->lpVtbl -> Render(This) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_SetSurfaceSize(This,width,height)	\
    ( (This)->lpVtbl -> SetSurfaceSize(This,width,height) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_SetFrame(This,theFrame)	\
    ( (This)->lpVtbl -> SetFrame(This,theFrame) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_Set3DPreviewFormat(This,previewFormat)	\
    ( (This)->lpVtbl -> Set3DPreviewFormat(This,previewFormat) ) 

#define IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_GetBackBuffer(This,backBuffer)	\
    ( (This)->lpVtbl -> GetBackBuffer(This,backBuffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoConversion_v14_2_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoConversion_v14_2_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoConversion_v14_2_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoConversion_v14_2_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3BBCB8A2-DA2C-42D9-B5D8-88083644E99A")
    IDeckLinkVideoConversion_v14_2_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConvertFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *srcFrame,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *dstFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoConversion_v14_2_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoConversion_v14_2_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoConversion_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoConversion_v14_2_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoConversion_v14_2_1, ConvertFrame)
        HRESULT ( STDMETHODCALLTYPE *ConvertFrame )( 
            IDeckLinkVideoConversion_v14_2_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *srcFrame,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *dstFrame);
        
        END_INTERFACE
    } IDeckLinkVideoConversion_v14_2_1Vtbl;

    interface IDeckLinkVideoConversion_v14_2_1
    {
        CONST_VTBL struct IDeckLinkVideoConversion_v14_2_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoConversion_v14_2_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoConversion_v14_2_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoConversion_v14_2_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoConversion_v14_2_1_ConvertFrame(This,srcFrame,dstFrame)	\
    ( (This)->lpVtbl -> ConvertFrame(This,srcFrame,dstFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoConversion_v14_2_1_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CDeckLinkGLScreenPreviewHelper_v14_2_1;

#ifdef __cplusplus

class DECLSPEC_UUID("F63E77C7-B655-4A4A-9AD0-3CA85D394343")
CDeckLinkGLScreenPreviewHelper_v14_2_1;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkGL3ScreenPreviewHelper_v14_2_1;

#ifdef __cplusplus

class DECLSPEC_UUID("00696A71-EBC7-491F-AC02-18D3393F33F0")
CDeckLinkGL3ScreenPreviewHelper_v14_2_1;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkDX9ScreenPreviewHelper_v14_2_1;

#ifdef __cplusplus

class DECLSPEC_UUID("CC010023-E01D-4525-9D59-80C8AB3DC7A0")
CDeckLinkDX9ScreenPreviewHelper_v14_2_1;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1;

#ifdef __cplusplus

class DECLSPEC_UUID("EF2A8478-7DDF-11E9-8F9E-2A86E4085A59")
CDeckLinkWPFDX9ScreenPreviewHelper_v14_2_1;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkVideoConversion_v14_2_1;

#ifdef __cplusplus

class DECLSPEC_UUID("7DBBBB11-5B7B-467D-AEA4-CEA468FD368C")
CDeckLinkVideoConversion_v14_2_1;
#endif

#ifndef __IDeckLinkInputCallback_v11_5_1_INTERFACE_DEFINED__
#define __IDeckLinkInputCallback_v11_5_1_INTERFACE_DEFINED__

/* interface IDeckLinkInputCallback_v11_5_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInputCallback_v11_5_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DD04E5EC-7415-42AB-AE4A-E80C4DFC044A")
    IDeckLinkInputCallback_v11_5_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged( 
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived( 
            /* [in] */ IDeckLinkVideoInputFrame_v14_2_1 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputCallback_v11_5_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInputCallback_v11_5_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInputCallback_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInputCallback_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInputCallback_v11_5_1, VideoInputFormatChanged)
        HRESULT ( STDMETHODCALLTYPE *VideoInputFormatChanged )( 
            IDeckLinkInputCallback_v11_5_1 * This,
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInputCallback_v11_5_1, VideoInputFrameArrived)
        HRESULT ( STDMETHODCALLTYPE *VideoInputFrameArrived )( 
            IDeckLinkInputCallback_v11_5_1 * This,
            /* [in] */ IDeckLinkVideoInputFrame_v14_2_1 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket);
        
        END_INTERFACE
    } IDeckLinkInputCallback_v11_5_1Vtbl;

    interface IDeckLinkInputCallback_v11_5_1
    {
        CONST_VTBL struct IDeckLinkInputCallback_v11_5_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInputCallback_v11_5_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInputCallback_v11_5_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInputCallback_v11_5_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInputCallback_v11_5_1_VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags)	\
    ( (This)->lpVtbl -> VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags) ) 

#define IDeckLinkInputCallback_v11_5_1_VideoInputFrameArrived(This,videoFrame,audioPacket)	\
    ( (This)->lpVtbl -> VideoInputFrameArrived(This,videoFrame,audioPacket) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInputCallback_v11_5_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_v11_5_1_INTERFACE_DEFINED__
#define __IDeckLinkInput_v11_5_1_INTERFACE_DEFINED__

/* interface IDeckLinkInput_v11_5_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput_v11_5_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9434C6E4-B15D-4B1C-979E-661E3DDCB4B9")
    IDeckLinkInput_v11_5_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoInputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoFrameCount( 
            /* [out] */ unsigned int *availableFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoInputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback_v11_5_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInput_v11_5_1Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDVideoInputConversionMode conversionMode,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, GetAvailableVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableVideoFrameCount )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [out] */ unsigned int *availableFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, SetVideoInputFrameMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetVideoInputFrameMemoryAllocator )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkInput_v11_5_1 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ IDeckLinkInputCallback_v11_5_1 *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_5_1, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkInput_v11_5_1 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkInput_v11_5_1Vtbl;

    interface IDeckLinkInput_v11_5_1
    {
        CONST_VTBL struct IDeckLinkInput_v11_5_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_v11_5_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_v11_5_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_v11_5_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_v11_5_1_DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,conversionMode,flags,actualMode,supported) ) 

#define IDeckLinkInput_v11_5_1_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkInput_v11_5_1_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_v11_5_1_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkInput_v11_5_1_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_v11_5_1_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_v11_5_1_GetAvailableVideoFrameCount(This,availableFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableVideoFrameCount(This,availableFrameCount) ) 

#define IDeckLinkInput_v11_5_1_SetVideoInputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoInputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkInput_v11_5_1_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_v11_5_1_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_v11_5_1_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkInput_v11_5_1_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_v11_5_1_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_v11_5_1_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_v11_5_1_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkInput_v11_5_1_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkInput_v11_5_1_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_v11_5_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkConfiguration_v10_11_INTERFACE_DEFINED__
#define __IDeckLinkConfiguration_v10_11_INTERFACE_DEFINED__

/* interface IDeckLinkConfiguration_v10_11 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkConfiguration_v10_11;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EF90380B-4AE5-4346-9077-E288E149F129")
    IDeckLinkConfiguration_v10_11 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteConfigurationToPreferences( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkConfiguration_v10_11Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkConfiguration_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkConfiguration_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkConfiguration_v10_11 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_11, WriteConfigurationToPreferences)
        HRESULT ( STDMETHODCALLTYPE *WriteConfigurationToPreferences )( 
            IDeckLinkConfiguration_v10_11 * This);
        
        END_INTERFACE
    } IDeckLinkConfiguration_v10_11Vtbl;

    interface IDeckLinkConfiguration_v10_11
    {
        CONST_VTBL struct IDeckLinkConfiguration_v10_11Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkConfiguration_v10_11_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkConfiguration_v10_11_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkConfiguration_v10_11_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkConfiguration_v10_11_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_11_WriteConfigurationToPreferences(This)	\
    ( (This)->lpVtbl -> WriteConfigurationToPreferences(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkConfiguration_v10_11_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAttributes_v10_11_INTERFACE_DEFINED__
#define __IDeckLinkAttributes_v10_11_INTERFACE_DEFINED__

/* interface IDeckLinkAttributes_v10_11 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAttributes_v10_11;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ABC11843-D966-44CB-96E2-A1CB5D3135C4")
    IDeckLinkAttributes_v10_11 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkAttributes_v10_11Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAttributes_v10_11 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAttributes_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAttributes_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkAttributes_v10_11, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkAttributes_v10_11 * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkAttributes_v10_11, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkAttributes_v10_11 * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkAttributes_v10_11, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkAttributes_v10_11 * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkAttributes_v10_11, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkAttributes_v10_11 * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkAttributes_v10_11Vtbl;

    interface IDeckLinkAttributes_v10_11
    {
        CONST_VTBL struct IDeckLinkAttributes_v10_11Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAttributes_v10_11_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAttributes_v10_11_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAttributes_v10_11_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAttributes_v10_11_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkAttributes_v10_11_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkAttributes_v10_11_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkAttributes_v10_11_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAttributes_v10_11_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkNotification_v10_11_INTERFACE_DEFINED__
#define __IDeckLinkNotification_v10_11_INTERFACE_DEFINED__

/* interface IDeckLinkNotification_v10_11 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkNotification_v10_11;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0A1FB207-E215-441B-9B19-6FA1575946C5")
    IDeckLinkNotification_v10_11 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Subscribe( 
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unsubscribe( 
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkNotification_v10_11Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkNotification_v10_11 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkNotification_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkNotification_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkNotification_v10_11, Subscribe)
        HRESULT ( STDMETHODCALLTYPE *Subscribe )( 
            IDeckLinkNotification_v10_11 * This,
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkNotification_v10_11, Unsubscribe)
        HRESULT ( STDMETHODCALLTYPE *Unsubscribe )( 
            IDeckLinkNotification_v10_11 * This,
            /* [in] */ BMDNotifications topic,
            /* [in] */ IDeckLinkNotificationCallback *theCallback);
        
        END_INTERFACE
    } IDeckLinkNotification_v10_11Vtbl;

    interface IDeckLinkNotification_v10_11
    {
        CONST_VTBL struct IDeckLinkNotification_v10_11Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkNotification_v10_11_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkNotification_v10_11_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkNotification_v10_11_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkNotification_v10_11_Subscribe(This,topic,theCallback)	\
    ( (This)->lpVtbl -> Subscribe(This,topic,theCallback) ) 

#define IDeckLinkNotification_v10_11_Unsubscribe(This,topic,theCallback)	\
    ( (This)->lpVtbl -> Unsubscribe(This,topic,theCallback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkNotification_v10_11_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkOutput_v10_11_INTERFACE_DEFINED__
#define __IDeckLinkOutput_v10_11_INTERFACE_DEFINED__

/* interface IDeckLinkOutput_v10_11 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkOutput_v10_11;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC5C8A6E-3F2F-4B3A-87EA-FD78AF300564")
    IDeckLinkOutput_v10_11 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoOutputFlags flags,
            /* [out] */ BMDDisplayModeSupport_v10_11 *result,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoOutput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrame( 
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame_v14_2_1 **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAncillaryData( 
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayVideoFrameSync( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleVideoFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduledFrameCompletionCallback( 
            /* [in] */ IDeckLinkVideoOutputCallback_v14_2_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedVideoFrameCount( 
            /* [out] */ unsigned int *bufferedFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioOutput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAudioSamplesSync( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleAudioSamples( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned int *bufferedSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushBufferedAudioSamples( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioCallback( 
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartScheduledPlayback( 
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopScheduledPlayback( 
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsScheduledPlaybackRunning( 
            /* [out] */ BOOL *active) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScheduledStreamTime( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferenceStatus( 
            /* [out] */ BMDReferenceStatus *referenceStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameCompletionReferenceTimestamp( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkOutput_v10_11Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkOutput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkOutput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoOutputFlags flags,
            /* [out] */ BMDDisplayModeSupport_v10_11 *result,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkOutput_v10_11 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, EnableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoOutput )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, DisableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoOutput )( 
            IDeckLinkOutput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, SetVideoOutputFrameMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputFrameMemoryAllocator )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, CreateVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrame )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame_v14_2_1 **outFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, CreateAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *CreateAncillaryData )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, DisplayVideoFrameSync)
        HRESULT ( STDMETHODCALLTYPE *DisplayVideoFrameSync )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, ScheduleVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *ScheduleVideoFrame )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, SetScheduledFrameCompletionCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScheduledFrameCompletionCallback )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ IDeckLinkVideoOutputCallback_v14_2_1 *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, GetBufferedVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedVideoFrameCount )( 
            IDeckLinkOutput_v10_11 * This,
            /* [out] */ unsigned int *bufferedFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, EnableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioOutput )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, DisableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioOutput )( 
            IDeckLinkOutput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, WriteAudioSamplesSync)
        HRESULT ( STDMETHODCALLTYPE *WriteAudioSamplesSync )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, BeginAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *BeginAudioPreroll )( 
            IDeckLinkOutput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, EndAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *EndAudioPreroll )( 
            IDeckLinkOutput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, ScheduleAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *ScheduleAudioSamples )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, GetBufferedAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkOutput_v10_11 * This,
            /* [out] */ unsigned int *bufferedSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, FlushBufferedAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *FlushBufferedAudioSamples )( 
            IDeckLinkOutput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, SetAudioCallback)
        HRESULT ( STDMETHODCALLTYPE *SetAudioCallback )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, StartScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StartScheduledPlayback )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, StopScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StopScheduledPlayback )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, IsScheduledPlaybackRunning)
        HRESULT ( STDMETHODCALLTYPE *IsScheduledPlaybackRunning )( 
            IDeckLinkOutput_v10_11 * This,
            /* [out] */ BOOL *active);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, GetScheduledStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetScheduledStreamTime )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, GetReferenceStatus)
        HRESULT ( STDMETHODCALLTYPE *GetReferenceStatus )( 
            IDeckLinkOutput_v10_11 * This,
            /* [out] */ BMDReferenceStatus *referenceStatus);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v10_11, GetFrameCompletionReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetFrameCompletionReferenceTimestamp )( 
            IDeckLinkOutput_v10_11 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp);
        
        END_INTERFACE
    } IDeckLinkOutput_v10_11Vtbl;

    interface IDeckLinkOutput_v10_11
    {
        CONST_VTBL struct IDeckLinkOutput_v10_11Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkOutput_v10_11_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkOutput_v10_11_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkOutput_v10_11_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkOutput_v10_11_DoesSupportVideoMode(This,displayMode,pixelFormat,flags,result,resultDisplayMode)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,flags,result,resultDisplayMode) ) 

#define IDeckLinkOutput_v10_11_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkOutput_v10_11_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkOutput_v10_11_EnableVideoOutput(This,displayMode,flags)	\
    ( (This)->lpVtbl -> EnableVideoOutput(This,displayMode,flags) ) 

#define IDeckLinkOutput_v10_11_DisableVideoOutput(This)	\
    ( (This)->lpVtbl -> DisableVideoOutput(This) ) 

#define IDeckLinkOutput_v10_11_SetVideoOutputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoOutputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkOutput_v10_11_CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_v10_11_CreateAncillaryData(This,pixelFormat,outBuffer)	\
    ( (This)->lpVtbl -> CreateAncillaryData(This,pixelFormat,outBuffer) ) 

#define IDeckLinkOutput_v10_11_DisplayVideoFrameSync(This,theFrame)	\
    ( (This)->lpVtbl -> DisplayVideoFrameSync(This,theFrame) ) 

#define IDeckLinkOutput_v10_11_ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale)	\
    ( (This)->lpVtbl -> ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale) ) 

#define IDeckLinkOutput_v10_11_SetScheduledFrameCompletionCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetScheduledFrameCompletionCallback(This,theCallback) ) 

#define IDeckLinkOutput_v10_11_GetBufferedVideoFrameCount(This,bufferedFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedVideoFrameCount(This,bufferedFrameCount) ) 

#define IDeckLinkOutput_v10_11_EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType)	\
    ( (This)->lpVtbl -> EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType) ) 

#define IDeckLinkOutput_v10_11_DisableAudioOutput(This)	\
    ( (This)->lpVtbl -> DisableAudioOutput(This) ) 

#define IDeckLinkOutput_v10_11_WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten)	\
    ( (This)->lpVtbl -> WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten) ) 

#define IDeckLinkOutput_v10_11_BeginAudioPreroll(This)	\
    ( (This)->lpVtbl -> BeginAudioPreroll(This) ) 

#define IDeckLinkOutput_v10_11_EndAudioPreroll(This)	\
    ( (This)->lpVtbl -> EndAudioPreroll(This) ) 

#define IDeckLinkOutput_v10_11_ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten)	\
    ( (This)->lpVtbl -> ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten) ) 

#define IDeckLinkOutput_v10_11_GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount) ) 

#define IDeckLinkOutput_v10_11_FlushBufferedAudioSamples(This)	\
    ( (This)->lpVtbl -> FlushBufferedAudioSamples(This) ) 

#define IDeckLinkOutput_v10_11_SetAudioCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetAudioCallback(This,theCallback) ) 

#define IDeckLinkOutput_v10_11_StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed)	\
    ( (This)->lpVtbl -> StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed) ) 

#define IDeckLinkOutput_v10_11_StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale)	\
    ( (This)->lpVtbl -> StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale) ) 

#define IDeckLinkOutput_v10_11_IsScheduledPlaybackRunning(This,active)	\
    ( (This)->lpVtbl -> IsScheduledPlaybackRunning(This,active) ) 

#define IDeckLinkOutput_v10_11_GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed)	\
    ( (This)->lpVtbl -> GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed) ) 

#define IDeckLinkOutput_v10_11_GetReferenceStatus(This,referenceStatus)	\
    ( (This)->lpVtbl -> GetReferenceStatus(This,referenceStatus) ) 

#define IDeckLinkOutput_v10_11_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#define IDeckLinkOutput_v10_11_GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp)	\
    ( (This)->lpVtbl -> GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkOutput_v10_11_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_v10_11_INTERFACE_DEFINED__
#define __IDeckLinkInput_v10_11_INTERFACE_DEFINED__

/* interface IDeckLinkInput_v10_11 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput_v10_11;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AF22762B-DFAC-4846-AA79-FA8883560995")
    IDeckLinkInput_v10_11 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags,
            /* [out] */ BMDDisplayModeSupport_v10_11 *result,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoFrameCount( 
            /* [out] */ unsigned int *availableFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoInputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback_v11_5_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInput_v10_11Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags,
            /* [out] */ BMDDisplayModeSupport_v10_11 *result,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput_v10_11 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, GetAvailableVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableVideoFrameCount )( 
            IDeckLinkInput_v10_11 * This,
            /* [out] */ unsigned int *availableFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, SetVideoInputFrameMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetVideoInputFrameMemoryAllocator )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkInput_v10_11 * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ IDeckLinkInputCallback_v11_5_1 *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v10_11, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkInput_v10_11 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkInput_v10_11Vtbl;

    interface IDeckLinkInput_v10_11
    {
        CONST_VTBL struct IDeckLinkInput_v10_11Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_v10_11_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_v10_11_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_v10_11_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_v10_11_DoesSupportVideoMode(This,displayMode,pixelFormat,flags,result,resultDisplayMode)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,flags,result,resultDisplayMode) ) 

#define IDeckLinkInput_v10_11_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_v10_11_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkInput_v10_11_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_v10_11_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_v10_11_GetAvailableVideoFrameCount(This,availableFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableVideoFrameCount(This,availableFrameCount) ) 

#define IDeckLinkInput_v10_11_SetVideoInputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoInputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkInput_v10_11_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_v10_11_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_v10_11_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkInput_v10_11_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_v10_11_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_v10_11_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_v10_11_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkInput_v10_11_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkInput_v10_11_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_v10_11_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkEncoderInput_v10_11_INTERFACE_DEFINED__
#define __IDeckLinkEncoderInput_v10_11_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderInput_v10_11 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderInput_v10_11;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("270587DA-6B7D-42E7-A1F0-6D853F581185")
    IDeckLinkEncoderInput_v10_11 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags,
            /* [out] */ BMDDisplayModeSupport_v10_11 *result,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailablePacketsCount( 
            /* [out] */ unsigned int *availablePacketsCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioFormat audioFormat,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkEncoderInputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderInput_v10_11Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags,
            /* [out] */ BMDDisplayModeSupport_v10_11 *result,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, GetAvailablePacketsCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailablePacketsCount )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [out] */ unsigned int *availablePacketsCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, SetMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetMemoryAllocator )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [in] */ BMDAudioFormat audioFormat,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkEncoderInput_v10_11 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [in] */ IDeckLinkEncoderInputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderInput_v10_11, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkEncoderInput_v10_11 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkEncoderInput_v10_11Vtbl;

    interface IDeckLinkEncoderInput_v10_11
    {
        CONST_VTBL struct IDeckLinkEncoderInput_v10_11Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderInput_v10_11_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderInput_v10_11_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderInput_v10_11_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderInput_v10_11_DoesSupportVideoMode(This,displayMode,pixelFormat,flags,result,resultDisplayMode)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,flags,result,resultDisplayMode) ) 

#define IDeckLinkEncoderInput_v10_11_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkEncoderInput_v10_11_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkEncoderInput_v10_11_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkEncoderInput_v10_11_GetAvailablePacketsCount(This,availablePacketsCount)	\
    ( (This)->lpVtbl -> GetAvailablePacketsCount(This,availablePacketsCount) ) 

#define IDeckLinkEncoderInput_v10_11_SetMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkEncoderInput_v10_11_EnableAudioInput(This,audioFormat,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,audioFormat,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkEncoderInput_v10_11_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkEncoderInput_v10_11_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkEncoderInput_v10_11_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkEncoderInput_v10_11_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkEncoderInput_v10_11_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkEncoderInput_v10_11_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkEncoderInput_v10_11_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkEncoderInput_v10_11_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderInput_v10_11_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CDeckLinkIterator_v10_11;

#ifdef __cplusplus

class DECLSPEC_UUID("87D2693F-8D4A-45C7-B43F-10ACBA25E68F")
CDeckLinkIterator_v10_11;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkDiscovery_v10_11;

#ifdef __cplusplus

class DECLSPEC_UUID("652615D4-26CD-4514-B161-2FD5072ED008")
CDeckLinkDiscovery_v10_11;
#endif

#ifndef __IDeckLinkConfiguration_v10_9_INTERFACE_DEFINED__
#define __IDeckLinkConfiguration_v10_9_INTERFACE_DEFINED__

/* interface IDeckLinkConfiguration_v10_9 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkConfiguration_v10_9;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CB71734A-FE37-4E8D-8E13-802133A1C3F2")
    IDeckLinkConfiguration_v10_9 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteConfigurationToPreferences( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkConfiguration_v10_9Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkConfiguration_v10_9 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkConfiguration_v10_9 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkConfiguration_v10_9 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_9, WriteConfigurationToPreferences)
        HRESULT ( STDMETHODCALLTYPE *WriteConfigurationToPreferences )( 
            IDeckLinkConfiguration_v10_9 * This);
        
        END_INTERFACE
    } IDeckLinkConfiguration_v10_9Vtbl;

    interface IDeckLinkConfiguration_v10_9
    {
        CONST_VTBL struct IDeckLinkConfiguration_v10_9Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkConfiguration_v10_9_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkConfiguration_v10_9_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkConfiguration_v10_9_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkConfiguration_v10_9_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_9_WriteConfigurationToPreferences(This)	\
    ( (This)->lpVtbl -> WriteConfigurationToPreferences(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkConfiguration_v10_9_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CBMDStreamingDiscovery_v10_8;

#ifdef __cplusplus

class DECLSPEC_UUID("0CAA31F6-8A26-40B0-86A4-BF58DCCA710C")
CBMDStreamingDiscovery_v10_8;
#endif

#ifndef __IDeckLinkConfiguration_v10_4_INTERFACE_DEFINED__
#define __IDeckLinkConfiguration_v10_4_INTERFACE_DEFINED__

/* interface IDeckLinkConfiguration_v10_4 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkConfiguration_v10_4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1E69FCF6-4203-4936-8076-2A9F4CFD50CB")
    IDeckLinkConfiguration_v10_4 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteConfigurationToPreferences( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkConfiguration_v10_4Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkConfiguration_v10_4 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkConfiguration_v10_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkConfiguration_v10_4 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_4, WriteConfigurationToPreferences)
        HRESULT ( STDMETHODCALLTYPE *WriteConfigurationToPreferences )( 
            IDeckLinkConfiguration_v10_4 * This);
        
        END_INTERFACE
    } IDeckLinkConfiguration_v10_4Vtbl;

    interface IDeckLinkConfiguration_v10_4
    {
        CONST_VTBL struct IDeckLinkConfiguration_v10_4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkConfiguration_v10_4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkConfiguration_v10_4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkConfiguration_v10_4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkConfiguration_v10_4_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_4_WriteConfigurationToPreferences(This)	\
    ( (This)->lpVtbl -> WriteConfigurationToPreferences(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkConfiguration_v10_4_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkConfiguration_v10_2_INTERFACE_DEFINED__
#define __IDeckLinkConfiguration_v10_2_INTERFACE_DEFINED__

/* interface IDeckLinkConfiguration_v10_2 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkConfiguration_v10_2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C679A35B-610C-4D09-B748-1D0478100FC0")
    IDeckLinkConfiguration_v10_2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteConfigurationToPreferences( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkConfiguration_v10_2Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkConfiguration_v10_2 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkConfiguration_v10_2 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkConfiguration_v10_2 * This,
            /* [in] */ BMDDeckLinkConfigurationID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkConfiguration_v10_2, WriteConfigurationToPreferences)
        HRESULT ( STDMETHODCALLTYPE *WriteConfigurationToPreferences )( 
            IDeckLinkConfiguration_v10_2 * This);
        
        END_INTERFACE
    } IDeckLinkConfiguration_v10_2Vtbl;

    interface IDeckLinkConfiguration_v10_2
    {
        CONST_VTBL struct IDeckLinkConfiguration_v10_2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkConfiguration_v10_2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkConfiguration_v10_2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkConfiguration_v10_2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkConfiguration_v10_2_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IDeckLinkConfiguration_v10_2_WriteConfigurationToPreferences(This)	\
    ( (This)->lpVtbl -> WriteConfigurationToPreferences(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkConfiguration_v10_2_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrameMetadataExtensions_v11_5_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrameMetadataExtensions_v11_5_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrameMetadataExtensions_v11_5 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrameMetadataExtensions_v11_5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D5973DC9-6432-46D0-8F0B-2496F8A1238F")
    IDeckLinkVideoFrameMetadataExtensions_v11_5 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameMetadataExtensions_v11_5Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrameMetadataExtensions_v11_5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrameMetadataExtensions_v11_5 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrameMetadataExtensions_v11_5 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions_v11_5, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkVideoFrameMetadataExtensions_v11_5 * This,
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions_v11_5, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkVideoFrameMetadataExtensions_v11_5 * This,
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions_v11_5, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkVideoFrameMetadataExtensions_v11_5 * This,
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkVideoFrameMetadataExtensions_v11_5, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkVideoFrameMetadataExtensions_v11_5 * This,
            /* [in] */ BMDDeckLinkFrameMetadataID_v11_5 metadataID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkVideoFrameMetadataExtensions_v11_5Vtbl;

    interface IDeckLinkVideoFrameMetadataExtensions_v11_5
    {
        CONST_VTBL struct IDeckLinkVideoFrameMetadataExtensions_v11_5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrameMetadataExtensions_v11_5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrameMetadataExtensions_v11_5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrameMetadataExtensions_v11_5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrameMetadataExtensions_v11_5_GetInt(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetInt(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMetadataExtensions_v11_5_GetFloat(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMetadataExtensions_v11_5_GetFlag(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,metadataID,value) ) 

#define IDeckLinkVideoFrameMetadataExtensions_v11_5_GetString(This,metadataID,value)	\
    ( (This)->lpVtbl -> GetString(This,metadataID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrameMetadataExtensions_v11_5_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkOutput_v11_4_INTERFACE_DEFINED__
#define __IDeckLinkOutput_v11_4_INTERFACE_DEFINED__

/* interface IDeckLinkOutput_v11_4 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkOutput_v11_4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("065A0F6C-C508-4D0D-B919-F5EB0EBFC96B")
    IDeckLinkOutput_v11_4 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoOutput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrame( 
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame_v14_2_1 **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAncillaryData( 
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayVideoFrameSync( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleVideoFrame( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduledFrameCompletionCallback( 
            /* [in] */ IDeckLinkVideoOutputCallback_v14_2_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedVideoFrameCount( 
            /* [out] */ unsigned int *bufferedFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioOutput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAudioSamplesSync( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleAudioSamples( 
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned int *bufferedSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushBufferedAudioSamples( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioCallback( 
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartScheduledPlayback( 
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopScheduledPlayback( 
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsScheduledPlaybackRunning( 
            /* [out] */ BOOL *active) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScheduledStreamTime( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferenceStatus( 
            /* [out] */ BMDReferenceStatus *referenceStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameCompletionReferenceTimestamp( 
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkOutput_v11_4Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkOutput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkOutput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BMDDisplayMode *actualMode,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkOutput_v11_4 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, EnableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoOutput )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDVideoOutputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, DisableVideoOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoOutput )( 
            IDeckLinkOutput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, SetVideoOutputFrameMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputFrameMemoryAllocator )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, CreateVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrame )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ int width,
            /* [in] */ int height,
            /* [in] */ int rowBytes,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame_v14_2_1 **outFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, CreateAncillaryData)
        HRESULT ( STDMETHODCALLTYPE *CreateAncillaryData )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, DisplayVideoFrameSync)
        HRESULT ( STDMETHODCALLTYPE *DisplayVideoFrameSync )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, ScheduleVideoFrame)
        HRESULT ( STDMETHODCALLTYPE *ScheduleVideoFrame )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeValue displayTime,
            /* [in] */ BMDTimeValue displayDuration,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, SetScheduledFrameCompletionCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScheduledFrameCompletionCallback )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ IDeckLinkVideoOutputCallback_v14_2_1 *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetBufferedVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedVideoFrameCount )( 
            IDeckLinkOutput_v11_4 * This,
            /* [out] */ unsigned int *bufferedFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, EnableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioOutput )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount,
            /* [in] */ BMDAudioOutputStreamType streamType);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, DisableAudioOutput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioOutput )( 
            IDeckLinkOutput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, WriteAudioSamplesSync)
        HRESULT ( STDMETHODCALLTYPE *WriteAudioSamplesSync )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, BeginAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *BeginAudioPreroll )( 
            IDeckLinkOutput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, EndAudioPreroll)
        HRESULT ( STDMETHODCALLTYPE *EndAudioPreroll )( 
            IDeckLinkOutput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, ScheduleAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *ScheduleAudioSamples )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int sampleFrameCount,
            /* [in] */ BMDTimeValue streamTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [out] */ unsigned int *sampleFramesWritten);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetBufferedAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkOutput_v11_4 * This,
            /* [out] */ unsigned int *bufferedSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, FlushBufferedAudioSamples)
        HRESULT ( STDMETHODCALLTYPE *FlushBufferedAudioSamples )( 
            IDeckLinkOutput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, SetAudioCallback)
        HRESULT ( STDMETHODCALLTYPE *SetAudioCallback )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, StartScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StartScheduledPlayback )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDTimeValue playbackStartTime,
            /* [in] */ BMDTimeScale timeScale,
            /* [in] */ double playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, StopScheduledPlayback)
        HRESULT ( STDMETHODCALLTYPE *StopScheduledPlayback )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            /* [in] */ BMDTimeScale timeScale);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, IsScheduledPlaybackRunning)
        HRESULT ( STDMETHODCALLTYPE *IsScheduledPlaybackRunning )( 
            IDeckLinkOutput_v11_4 * This,
            /* [out] */ BOOL *active);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetScheduledStreamTime)
        HRESULT ( STDMETHODCALLTYPE *GetScheduledStreamTime )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetReferenceStatus)
        HRESULT ( STDMETHODCALLTYPE *GetReferenceStatus )( 
            IDeckLinkOutput_v11_4 * This,
            /* [out] */ BMDReferenceStatus *referenceStatus);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        DECLSPEC_XFGVIRT(IDeckLinkOutput_v11_4, GetFrameCompletionReferenceTimestamp)
        HRESULT ( STDMETHODCALLTYPE *GetFrameCompletionReferenceTimestamp )( 
            IDeckLinkOutput_v11_4 * This,
            /* [in] */ IDeckLinkVideoFrame_v14_2_1 *theFrame,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *frameCompletionTimestamp);
        
        END_INTERFACE
    } IDeckLinkOutput_v11_4Vtbl;

    interface IDeckLinkOutput_v11_4
    {
        CONST_VTBL struct IDeckLinkOutput_v11_4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkOutput_v11_4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkOutput_v11_4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkOutput_v11_4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkOutput_v11_4_DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,flags,actualMode,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,flags,actualMode,supported) ) 

#define IDeckLinkOutput_v11_4_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkOutput_v11_4_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkOutput_v11_4_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkOutput_v11_4_EnableVideoOutput(This,displayMode,flags)	\
    ( (This)->lpVtbl -> EnableVideoOutput(This,displayMode,flags) ) 

#define IDeckLinkOutput_v11_4_DisableVideoOutput(This)	\
    ( (This)->lpVtbl -> DisableVideoOutput(This) ) 

#define IDeckLinkOutput_v11_4_SetVideoOutputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoOutputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkOutput_v11_4_CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_v11_4_CreateAncillaryData(This,pixelFormat,outBuffer)	\
    ( (This)->lpVtbl -> CreateAncillaryData(This,pixelFormat,outBuffer) ) 

#define IDeckLinkOutput_v11_4_DisplayVideoFrameSync(This,theFrame)	\
    ( (This)->lpVtbl -> DisplayVideoFrameSync(This,theFrame) ) 

#define IDeckLinkOutput_v11_4_ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale)	\
    ( (This)->lpVtbl -> ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale) ) 

#define IDeckLinkOutput_v11_4_SetScheduledFrameCompletionCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetScheduledFrameCompletionCallback(This,theCallback) ) 

#define IDeckLinkOutput_v11_4_GetBufferedVideoFrameCount(This,bufferedFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedVideoFrameCount(This,bufferedFrameCount) ) 

#define IDeckLinkOutput_v11_4_EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType)	\
    ( (This)->lpVtbl -> EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType) ) 

#define IDeckLinkOutput_v11_4_DisableAudioOutput(This)	\
    ( (This)->lpVtbl -> DisableAudioOutput(This) ) 

#define IDeckLinkOutput_v11_4_WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten)	\
    ( (This)->lpVtbl -> WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten) ) 

#define IDeckLinkOutput_v11_4_BeginAudioPreroll(This)	\
    ( (This)->lpVtbl -> BeginAudioPreroll(This) ) 

#define IDeckLinkOutput_v11_4_EndAudioPreroll(This)	\
    ( (This)->lpVtbl -> EndAudioPreroll(This) ) 

#define IDeckLinkOutput_v11_4_ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten)	\
    ( (This)->lpVtbl -> ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten) ) 

#define IDeckLinkOutput_v11_4_GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount) ) 

#define IDeckLinkOutput_v11_4_FlushBufferedAudioSamples(This)	\
    ( (This)->lpVtbl -> FlushBufferedAudioSamples(This) ) 

#define IDeckLinkOutput_v11_4_SetAudioCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetAudioCallback(This,theCallback) ) 

#define IDeckLinkOutput_v11_4_StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed)	\
    ( (This)->lpVtbl -> StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed) ) 

#define IDeckLinkOutput_v11_4_StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale)	\
    ( (This)->lpVtbl -> StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale) ) 

#define IDeckLinkOutput_v11_4_IsScheduledPlaybackRunning(This,active)	\
    ( (This)->lpVtbl -> IsScheduledPlaybackRunning(This,active) ) 

#define IDeckLinkOutput_v11_4_GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed)	\
    ( (This)->lpVtbl -> GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed) ) 

#define IDeckLinkOutput_v11_4_GetReferenceStatus(This,referenceStatus)	\
    ( (This)->lpVtbl -> GetReferenceStatus(This,referenceStatus) ) 

#define IDeckLinkOutput_v11_4_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#define IDeckLinkOutput_v11_4_GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp)	\
    ( (This)->lpVtbl -> GetFrameCompletionReferenceTimestamp(This,theFrame,desiredTimeScale,frameCompletionTimestamp) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkOutput_v11_4_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_v11_4_INTERFACE_DEFINED__
#define __IDeckLinkInput_v11_4_INTERFACE_DEFINED__

/* interface IDeckLinkInput_v11_4 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput_v11_4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2A88CF76-F494-4216-A7EF-DC74EEB83882")
    IDeckLinkInput_v11_4 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BOOL *supported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayMode( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoFrameCount( 
            /* [out] */ unsigned int *availableFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoInputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned int *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback_v11_5_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkInput_v11_4Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, DoesSupportVideoMode)
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ BMDVideoConnection connection,
            /* [in] */ BMDDisplayMode requestedMode,
            /* [in] */ BMDPixelFormat requestedPixelFormat,
            /* [in] */ BMDSupportedVideoModeFlags flags,
            /* [out] */ BOOL *supported);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, GetDisplayMode)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [out] */ IDeckLinkDisplayMode **resultDisplayMode);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, GetDisplayModeIterator)
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput_v11_4 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, SetScreenPreviewCallback)
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback_v14_2_1 *previewCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, EnableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ BMDDisplayMode displayMode,
            /* [in] */ BMDPixelFormat pixelFormat,
            /* [in] */ BMDVideoInputFlags flags);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, DisableVideoInput)
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, GetAvailableVideoFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableVideoFrameCount )( 
            IDeckLinkInput_v11_4 * This,
            /* [out] */ unsigned int *availableFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, SetVideoInputFrameMemoryAllocator)
        HRESULT ( STDMETHODCALLTYPE *SetVideoInputFrameMemoryAllocator )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ IDeckLinkMemoryAllocator_v14_2_1 *theAllocator);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, EnableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ BMDAudioSampleRate sampleRate,
            /* [in] */ BMDAudioSampleType sampleType,
            /* [in] */ unsigned int channelCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, DisableAudioInput)
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, GetAvailableAudioSampleFrameCount)
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkInput_v11_4 * This,
            /* [out] */ unsigned int *availableSampleFrameCount);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, StartStreams)
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, StopStreams)
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, PauseStreams)
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, FlushStreams)
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkInput_v11_4 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, SetCallback)
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ IDeckLinkInputCallback_v11_5_1 *theCallback);
        
        DECLSPEC_XFGVIRT(IDeckLinkInput_v11_4, GetHardwareReferenceClock)
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkInput_v11_4 * This,
            /* [in] */ BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkInput_v11_4Vtbl;

    interface IDeckLinkInput_v11_4
    {
        CONST_VTBL struct IDeckLinkInput_v11_4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_v11_4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_v11_4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_v11_4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_v11_4_DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,flags,supported)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,connection,requestedMode,requestedPixelFormat,flags,supported) ) 

#define IDeckLinkInput_v11_4_GetDisplayMode(This,displayMode,resultDisplayMode)	\
    ( (This)->lpVtbl -> GetDisplayMode(This,displayMode,resultDisplayMode) ) 

#define IDeckLinkInput_v11_4_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_v11_4_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkInput_v11_4_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_v11_4_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_v11_4_GetAvailableVideoFrameCount(This,availableFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableVideoFrameCount(This,availableFrameCount) ) 

#define IDeckLinkInput_v11_4_SetVideoInputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoInputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkInput_v11_4_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_v11_4_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_v11_4_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkInput_v11_4_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_v11_4_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_v11_4_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_v11_4_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkInput_v11_4_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkInput_v11_4_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_v11_4_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CDeckLinkIterator_v10_8;

#ifdef __cplusplus

class DECLSPEC_UUID("1F2E109A-8F4F-49E4-9203-135595CB6FA5")
CDeckLinkIterator_v10_8;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkDiscovery_v10_8;

#ifdef __cplusplus

class DECLSPEC_UUID("1073A05C-D885-47E9-B3C6-129B3F9F648B")
CDeckLinkDiscovery_v10_8;
#endif

#ifndef __IDeckLinkEncoderConfiguration_v10_5_INTERFACE_DEFINED__
#define __IDeckLinkEncoderConfiguration_v10_5_INTERFACE_DEFINED__

/* interface IDeckLinkEncoderConfiguration_v10_5 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkEncoderConfiguration_v10_5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("67455668-0848-45DF-8D8E-350A77C9A028")
    IDeckLinkEncoderConfiguration_v10_5 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlag( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BOOL value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInt( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ LONGLONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFloat( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ double value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDecoderConfigurationInfo( 
            /* [out] */ void *buffer,
            /* [in] */ long bufferSize,
            /* [out] */ long *returnedSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDeckLinkEncoderConfiguration_v10_5Vtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkEncoderConfiguration_v10_5 * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkEncoderConfiguration_v10_5 * This);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, SetFlag)
        HRESULT ( STDMETHODCALLTYPE *SetFlag )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BOOL value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, GetFlag)
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BOOL *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, SetInt)
        HRESULT ( STDMETHODCALLTYPE *SetInt )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ LONGLONG value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, GetInt)
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ LONGLONG *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, SetFloat)
        HRESULT ( STDMETHODCALLTYPE *SetFloat )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ double value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, GetFloat)
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ double *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, SetString)
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [in] */ BSTR value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, GetString)
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [in] */ BMDDeckLinkEncoderConfigurationID cfgID,
            /* [out] */ BSTR *value);
        
        DECLSPEC_XFGVIRT(IDeckLinkEncoderConfiguration_v10_5, GetDecoderConfigurationInfo)
        HRESULT ( STDMETHODCALLTYPE *GetDecoderConfigurationInfo )( 
            IDeckLinkEncoderConfiguration_v10_5 * This,
            /* [out] */ void *buffer,
            /* [in] */ long bufferSize,
            /* [out] */ long *returnedSize);
        
        END_INTERFACE
    } IDeckLinkEncoderConfiguration_v10_5Vtbl;

    interface IDeckLinkEncoderConfiguration_v10_5
    {
        CONST_VTBL struct IDeckLinkEncoderConfiguration_v10_5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkEncoderConfiguration_v10_5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkEncoderConfiguration_v10_5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkEncoderConfiguration_v10_5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkEncoderConfiguration_v10_5_SetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFlag(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_SetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetInt(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_SetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetFloat(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_SetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> SetString(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#define IDeckLinkEncoderConfiguration_v10_5_GetDecoderConfigurationInfo(This,buffer,bufferSize,returnedSize)	\
    ( (This)->lpVtbl -> GetDecoderConfigurationInfo(This,buffer,bufferSize,returnedSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkEncoderConfiguration_v10_5_INTERFACE_DEFINED__ */

#endif /* __DeckLinkAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


