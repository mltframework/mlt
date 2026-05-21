#!/usr/bin/env python3
"""
Script to generate filter_info.yml

Usage:
  python3 generate_filter_info.py --ffmpeg-dir PATH --output-yml PATH
"""
import argparse
import os
import re
import sys

# Will be set by parse_args()
FFMPEG_DIR = None
ALLFILTERS = None
OUT_YML = None

# =========================================================================
# Command-line argument parsing
# =========================================================================
def parse_args():
    """Parse and validate command-line arguments"""
    global FFMPEG_DIR, ALLFILTERS, OUT_YML
    
    parser = argparse.ArgumentParser(
        description="Parse FFmpeg libavfilter source and generate filter_info.yml with MLT format mappings"
    )
    parser.add_argument(
        "--ffmpeg-dir",
        required=True,
        help="Path to FFmpeg libavfilter directory"
    )
    parser.add_argument(
        "--output-yml",
        required=True,
        help="Output YAML file path"
    )
    
    args = parser.parse_args()
    
    # Set global variables
    FFMPEG_DIR = args.ffmpeg_dir
    ALLFILTERS = os.path.join(args.ffmpeg_dir, "allfilters.c")
    OUT_YML = args.output_yml
    
    # Validate paths
    if not os.path.isdir(FFMPEG_DIR):
        print(f"Error: FFmpeg directory not found: {FFMPEG_DIR}", file=sys.stderr)
        sys.exit(1)
    
    if not os.path.exists(ALLFILTERS):
        print(f"Error: allfilters.c not found: {ALLFILTERS}", file=sys.stderr)
        sys.exit(1)
    
    # Create output directory if needed
    output_dir = os.path.dirname(OUT_YML)
    if output_dir and not os.path.isdir(output_dir):
        try:
            os.makedirs(output_dir, exist_ok=True)
        except OSError as e:
            print(f"Error: Could not create output directory {output_dir}: {e}", file=sys.stderr)
            sys.exit(1)
    
    return args


# =========================================================================
# EMBEDDED MAPPINGS: FFmpeg -> MLT format mapping
# =========================================================================
FFMPEG_TO_MLT_MAPPINGS = {
    "audio_mapping": {
        "dbl": "f32le",
        "dblp": "f32le",
        "flt": "f32le",
        "fltp": "f32le",
        "none": "none",
        "s16": "s16",
        "s16p": "s16",
        "s32": "s32le",
        "s32p": "s32le",
        "s64": "s32le",
        "s64p": "s32le",
        "u8": "u8",
        "u8p": "u8",
    },
    "video_mapping": {
        "0bgr": "rgb",
        "0rgb": "rgb",
        "abgr": "rgba",
        "amf_surface": "none",
        "argb": "rgba",
        "ayuv": "yuv444p10",
        "ayuv64be": "yuv444p10",
        "ayuv64le": "yuv444p10",
        "bayer_bggr16be": "rgba64",
        "bayer_bggr16le": "rgba64",
        "bayer_bggr8": "rgb",
        "bayer_gbrg16be": "rgba64",
        "bayer_gbrg16le": "rgba64",
        "bayer_gbrg8": "rgb",
        "bayer_grbg16be": "rgba64",
        "bayer_grbg16le": "rgba64",
        "bayer_grbg8": "rgb",
        "bayer_rggb16be": "rgba64",
        "bayer_rggb16le": "rgba64",
        "bayer_rggb8": "rgb",
        "bgr0": "rgb",
        "bgr24": "rgb",
        "bgr4": "rgb",
        "bgr444be": "rgb",
        "bgr444le": "rgb",
        "bgr48be": "rgba64",
        "bgr48le": "rgba64",
        "bgr4_byte": "rgb",
        "bgr555be": "rgb",
        "bgr555le": "rgb",
        "bgr565be": "rgb",
        "bgr565le": "rgb",
        "bgr8": "rgb",
        "bgra": "rgba",
        "bgra64be": "rgba64",
        "bgra64le": "rgba64",
        "cuda": "none",
        "d3d11": "none",
        "d3d11va_vld": "none",
        "d3d12": "none",
        "drm_prime": "none",
        "dxva2_vld": "none",
        "gbr24p": "rgb",
        "gbrap": "rgba",
        "gbrap10be": "rgba64",
        "gbrap10le": "rgba64",
        "gbrap12be": "rgba64",
        "gbrap12le": "rgba64",
        "gbrap14be": "rgba64",
        "gbrap14le": "rgba64",
        "gbrap16be": "rgba64",
        "gbrap16le": "rgba64",
        "gbrap32be": "rgba64",
        "gbrap32le": "rgba64",
        "gbrapf16be": "rgba64",
        "gbrapf16le": "rgba64",
        "gbrapf32be": "rgba64",
        "gbrapf32le": "rgba64",
        "gbrp": "rgb",
        "gbrp10be": "rgba64",
        "gbrp10le": "rgba64",
        "gbrp10msbbe": "rgba64",
        "gbrp10msble": "rgba64",
        "gbrp12be": "rgba64",
        "gbrp12le": "rgba64",
        "gbrp12msbbe": "rgba64",
        "gbrp12msble": "rgba64",
        "gbrp14be": "rgba64",
        "gbrp14le": "rgba64",
        "gbrp16be": "rgba64",
        "gbrp16le": "rgba64",
        "gbrp9be": "rgba64",
        "gbrp9le": "rgba64",
        "gbrpf16be": "rgba64",
        "gbrpf16le": "rgba64",
        "gbrpf32be": "rgba64",
        "gbrpf32le": "rgba64",
        "gray10be": "rgba64",
        "gray10le": "rgba64",
        "gray12be": "rgba64",
        "gray12le": "rgba64",
        "gray14be": "rgba64",
        "gray14le": "rgba64",
        "gray16be": "rgba64",
        "gray16le": "rgba64",
        "gray32be": "rgba64",
        "gray32le": "rgba64",
        "gray8": "rgb",
        "gray8a": "rgb",
        "gray9be": "rgba64",
        "gray9le": "rgba64",
        "grayf16be": "rgba64",
        "grayf16le": "rgba64",
        "grayf32be": "rgba64",
        "grayf32le": "rgba64",
        "mediacodec": "none",
        "mmal": "none",
        "monoblack": "rgb",
        "monowhite": "rgb",
        "none": "none",
        "nv12": "yuv420p10",
        "nv16": "yuv422p16",
        "nv20be": "yuv422",
        "nv20le": "yuv422",
        "nv21": "yuv420p",
        "nv24": "yuv420p",
        "nv42": "yuv420p",
        "ohcodec": "none",
        "opencl": "none",
        "p010be": "yuv420p10",
        "p010le": "yuv420p10",
        "p012be": "yuv420p10",
        "p012le": "yuv420p10",
        "p016be": "yuv420p10",
        "p016le": "yuv420p10",
        "p210be": "yuv422p16",
        "p210le": "yuv422p16",
        "p212be": "yuv420p",
        "p212le": "yuv420p",
        "p216be": "yuv422p16",
        "p216le": "yuv422p16",
        "p410be": "none",
        "p410le": "none",
        "p412be": "none",
        "p412le": "none",
        "p416be": "none",
        "p416le": "none",
        "pal8": "rgba",
        "qsv": "none",
        "rgb0": "rgb",
        "rgb24": "rgb",
        "rgb4": "rgb",
        "rgb444be": "rgb",
        "rgb444le": "rgb",
        "rgb48be": "rgba64",
        "rgb48le": "rgba64",
        "rgb4_byte": "rgb",
        "rgb555be": "rgb",
        "rgb555le": "rgb",
        "rgb565be": "rgb",
        "rgb565le": "rgb",
        "rgb8": "rgb",
        "rgb96be": "rgba64",
        "rgb96le": "rgba64",
        "rgba": "rgba",
        "rgba128be": "rgba64",
        "rgba128le": "rgba64",
        "rgba64be": "rgba64",
        "rgba64le": "rgba64",
        "rgbaf16be": "rgba64",
        "rgbaf16le": "rgba64",
        "rgbaf32be": "rgba64",
        "rgbaf32le": "rgba64",
        "rgbf16be": "rgba64",
        "rgbf16le": "rgba64",
        "rgbf32be": "rgba64",
        "rgbf32le": "rgba64",
        "uyva": "none",
        "uyvy422": "yuv422",
        "uyyvyy411": "none",
        "v30xbe": "none",
        "v30xle": "none",
        "vaapi": "none",
        "vdpau": "none",
        "videotoolbox": "none",
        "vulkan": "none",
        "vuya": "none",
        "vuyx": "none",
        "vyu444": "none",
        "x2bgr10be": "rgba64",
        "x2bgr10le": "rgba64",
        "x2rgb10be": "rgba64",
        "x2rgb10le": "rgba64",
        "xv30be": "none",
        "xv30le": "none",
        "xv36be": "none",
        "xv36le": "none",
        "xv48be": "none",
        "xv48le": "none",
        "xyz12be": "rgba64",
        "xyz12le": "rgba64",
        "y210be": "yuv422p16",
        "y210le": "yuv422p16",
        "y212be": "yuv422p16",
        "y212le": "yuv422p16",
        "y216be": "yuv422p16",
        "y216le": "yuv422p16",
        "y400a": "rgb",
        "ya16be": "rgba64",
        "ya16le": "rgba64",
        "ya8": "rgb",
        "yaf16be": "rgba64",
        "yaf16le": "rgba64",
        "yaf32be": "rgba64",
        "yaf32le": "rgba64",
        "yuv410p": "yuv420p",
        "yuv411p": "yuv422",
        "yuv420p": "yuv420p",
        "yuv420p10be": "yuv420p10",
        "yuv420p10le": "yuv420p10",
        "yuv420p12be": "yuv420p10",
        "yuv420p12le": "yuv420p10",
        "yuv420p14be": "yuv420p10",
        "yuv420p14le": "yuv420p10",
        "yuv420p16be": "yuv420p10",
        "yuv420p16le": "yuv420p10",
        "yuv420p9be": "yuv420p10",
        "yuv420p9le": "yuv420p10",
        "yuv422p": "yuv422",
        "yuv422p10be": "yuv422p16",
        "yuv422p10le": "yuv422p16",
        "yuv422p12be": "yuv422p16",
        "yuv422p12le": "yuv422p16",
        "yuv422p14be": "yuv422p16",
        "yuv422p14le": "yuv422p16",
        "yuv422p16be": "yuv422p16",
        "yuv422p16le": "yuv422p16",
        "yuv422p9be": "yuv422p16",
        "yuv422p9le": "yuv422p16",
        "yuv440p": "yuv422",
        "yuv440p10be": "yuv422p16",
        "yuv440p10le": "yuv422p16",
        "yuv440p12be": "yuv422p16",
        "yuv440p12le": "yuv422p16",
        "yuv444p": "yuv444p10",
        "yuv444p10be": "yuv444p10",
        "yuv444p10le": "yuv444p10",
        "yuv444p10msbbe": "yuv444p10",
        "yuv444p10msble": "yuv444p10",
        "yuv444p12be": "yuv444p10",
        "yuv444p12le": "yuv444p10",
        "yuv444p12msbbe": "yuv444p10",
        "yuv444p12msble": "yuv444p10",
        "yuv444p14be": "yuv444p10",
        "yuv444p14le": "yuv444p10",
        "yuv444p16be": "yuv444p10",
        "yuv444p16le": "yuv444p10",
        "yuv444p9be": "yuv444p10",
        "yuv444p9le": "yuv444p10",
        "yuva420p": "yuv420p",
        "yuva420p10be": "yuv420p10",
        "yuva420p10le": "yuv420p10",
        "yuva420p16be": "yuv420p10",
        "yuva420p16le": "yuv420p10",
        "yuva420p9be": "yuv420p10",
        "yuva420p9le": "yuv420p10",
        "yuva422p": "yuv422",
        "yuva422p10be": "yuv422p16",
        "yuva422p10le": "yuv422p16",
        "yuva422p12be": "yuv422p16",
        "yuva422p12le": "yuv422p16",
        "yuva422p16be": "yuv422p16",
        "yuva422p16le": "yuv422p16",
        "yuva422p9be": "yuv422p16",
        "yuva422p9le": "yuv422p16",
        "yuva444p": "yuv444p10",
        "yuva444p10be": "yuv444p10",
        "yuva444p10le": "yuv444p10",
        "yuva444p12be": "yuv444p10",
        "yuva444p12le": "yuv444p10",
        "yuva444p16be": "yuv444p10",
        "yuva444p16le": "yuv444p10",
    },
}

# =========================================================================
# EMBEDDED DATA: FFmpeg audio/video formats comprehensive
# =========================================================================
FFMPEG_AUDIO_FORMATS = [
    "AV_SAMPLE_FMT_DBL",
    "AV_SAMPLE_FMT_DBLP",
    "AV_SAMPLE_FMT_FLT",
    "AV_SAMPLE_FMT_FLTP",
    "AV_SAMPLE_FMT_NONE",
    "AV_SAMPLE_FMT_S16",
    "AV_SAMPLE_FMT_S16P",
    "AV_SAMPLE_FMT_S32",
    "AV_SAMPLE_FMT_S32P",
    "AV_SAMPLE_FMT_S64",
    "AV_SAMPLE_FMT_S64P",
    "AV_SAMPLE_FMT_U8",
    "AV_SAMPLE_FMT_U8P",
]

FFMPEG_VIDEO_FORMATS = [
    "AV_PIX_FMT_0BGR",
    "AV_PIX_FMT_0RGB",
    "AV_PIX_FMT_ABGR",
    "AV_PIX_FMT_AMF_SURFACE",
    "AV_PIX_FMT_ARGB",
    "AV_PIX_FMT_AYUV",
    "AV_PIX_FMT_AYUV64BE",
    "AV_PIX_FMT_AYUV64LE",
    "AV_PIX_FMT_BAYER_BGGR16BE",
    "AV_PIX_FMT_BAYER_BGGR16LE",
    "AV_PIX_FMT_BAYER_BGGR8",
    "AV_PIX_FMT_BAYER_GBRG16BE",
    "AV_PIX_FMT_BAYER_GBRG16LE",
    "AV_PIX_FMT_BAYER_GBRG8",
    "AV_PIX_FMT_BAYER_GRBG16BE",
    "AV_PIX_FMT_BAYER_GRBG16LE",
    "AV_PIX_FMT_BAYER_GRBG8",
    "AV_PIX_FMT_BAYER_RGGB16BE",
    "AV_PIX_FMT_BAYER_RGGB16LE",
    "AV_PIX_FMT_BAYER_RGGB8",
    "AV_PIX_FMT_BGR0",
    "AV_PIX_FMT_BGR24",
    "AV_PIX_FMT_BGR4",
    "AV_PIX_FMT_BGR444BE",
    "AV_PIX_FMT_BGR444LE",
    "AV_PIX_FMT_BGR48BE",
    "AV_PIX_FMT_BGR48LE",
    "AV_PIX_FMT_BGR4_BYTE",
    "AV_PIX_FMT_BGR555BE",
    "AV_PIX_FMT_BGR555LE",
    "AV_PIX_FMT_BGR565BE",
    "AV_PIX_FMT_BGR565LE",
    "AV_PIX_FMT_BGR8",
    "AV_PIX_FMT_BGRA",
    "AV_PIX_FMT_BGRA64BE",
    "AV_PIX_FMT_BGRA64LE",
    "AV_PIX_FMT_CUDA",
    "AV_PIX_FMT_D3D11",
    "AV_PIX_FMT_D3D11VA_VLD",
    "AV_PIX_FMT_D3D12",
    "AV_PIX_FMT_DRM_PRIME",
    "AV_PIX_FMT_DXVA2_VLD",
    "AV_PIX_FMT_GBR24P",
    "AV_PIX_FMT_GBRAP",
    "AV_PIX_FMT_GBRAP10BE",
    "AV_PIX_FMT_GBRAP10LE",
    "AV_PIX_FMT_GBRAP12BE",
    "AV_PIX_FMT_GBRAP12LE",
    "AV_PIX_FMT_GBRAP14BE",
    "AV_PIX_FMT_GBRAP14LE",
    "AV_PIX_FMT_GBRAP16BE",
    "AV_PIX_FMT_GBRAP16LE",
    "AV_PIX_FMT_GBRAP32BE",
    "AV_PIX_FMT_GBRAP32LE",
    "AV_PIX_FMT_GBRAPF16BE",
    "AV_PIX_FMT_GBRAPF16LE",
    "AV_PIX_FMT_GBRAPF32BE",
    "AV_PIX_FMT_GBRAPF32LE",
    "AV_PIX_FMT_GBRP",
    "AV_PIX_FMT_GBRP10BE",
    "AV_PIX_FMT_GBRP10LE",
    "AV_PIX_FMT_GBRP10MSBBE",
    "AV_PIX_FMT_GBRP10MSBLE",
    "AV_PIX_FMT_GBRP12BE",
    "AV_PIX_FMT_GBRP12LE",
    "AV_PIX_FMT_GBRP12MSBBE",
    "AV_PIX_FMT_GBRP12MSBLE",
    "AV_PIX_FMT_GBRP14BE",
    "AV_PIX_FMT_GBRP14LE",
    "AV_PIX_FMT_GBRP16BE",
    "AV_PIX_FMT_GBRP16LE",
    "AV_PIX_FMT_GBRP9BE",
    "AV_PIX_FMT_GBRP9LE",
    "AV_PIX_FMT_GBRPF16BE",
    "AV_PIX_FMT_GBRPF16LE",
    "AV_PIX_FMT_GBRPF32BE",
    "AV_PIX_FMT_GBRPF32LE",
    "AV_PIX_FMT_GRAY10BE",
    "AV_PIX_FMT_GRAY10LE",
    "AV_PIX_FMT_GRAY12BE",
    "AV_PIX_FMT_GRAY12LE",
    "AV_PIX_FMT_GRAY14BE",
    "AV_PIX_FMT_GRAY14LE",
    "AV_PIX_FMT_GRAY16BE",
    "AV_PIX_FMT_GRAY16LE",
    "AV_PIX_FMT_GRAY32BE",
    "AV_PIX_FMT_GRAY32LE",
    "AV_PIX_FMT_GRAY8",
    "AV_PIX_FMT_GRAY8A",
    "AV_PIX_FMT_GRAY9BE",
    "AV_PIX_FMT_GRAY9LE",
    "AV_PIX_FMT_GRAYF16BE",
    "AV_PIX_FMT_GRAYF16LE",
    "AV_PIX_FMT_GRAYF32BE",
    "AV_PIX_FMT_GRAYF32LE",
    "AV_PIX_FMT_MEDIACODEC",
    "AV_PIX_FMT_MMAL",
    "AV_PIX_FMT_MONOBLACK",
    "AV_PIX_FMT_MONOWHITE",
    "AV_PIX_FMT_NONE",
    "AV_PIX_FMT_NV12",
    "AV_PIX_FMT_NV16",
    "AV_PIX_FMT_NV20BE",
    "AV_PIX_FMT_NV20LE",
    "AV_PIX_FMT_NV21",
    "AV_PIX_FMT_NV24",
    "AV_PIX_FMT_NV42",
    "AV_PIX_FMT_OHCODEC",
    "AV_PIX_FMT_OPENCL",
    "AV_PIX_FMT_P010BE",
    "AV_PIX_FMT_P010LE",
    "AV_PIX_FMT_P012BE",
    "AV_PIX_FMT_P012LE",
    "AV_PIX_FMT_P016BE",
    "AV_PIX_FMT_P016LE",
    "AV_PIX_FMT_P210BE",
    "AV_PIX_FMT_P210LE",
    "AV_PIX_FMT_P212BE",
    "AV_PIX_FMT_P212LE",
    "AV_PIX_FMT_P216BE",
    "AV_PIX_FMT_P216LE",
    "AV_PIX_FMT_P410BE",
    "AV_PIX_FMT_P410LE",
    "AV_PIX_FMT_P412BE",
    "AV_PIX_FMT_P412LE",
    "AV_PIX_FMT_P416BE",
    "AV_PIX_FMT_P416LE",
    "AV_PIX_FMT_PAL8",
    "AV_PIX_FMT_QSV",
    "AV_PIX_FMT_RGB0",
    "AV_PIX_FMT_RGB24",
    "AV_PIX_FMT_RGB4",
    "AV_PIX_FMT_RGB444BE",
    "AV_PIX_FMT_RGB444LE",
    "AV_PIX_FMT_RGB48BE",
    "AV_PIX_FMT_RGB48LE",
    "AV_PIX_FMT_RGB4_BYTE",
    "AV_PIX_FMT_RGB555BE",
    "AV_PIX_FMT_RGB555LE",
    "AV_PIX_FMT_RGB565BE",
    "AV_PIX_FMT_RGB565LE",
    "AV_PIX_FMT_RGB8",
    "AV_PIX_FMT_RGB96BE",
    "AV_PIX_FMT_RGB96LE",
    "AV_PIX_FMT_RGBA",
    "AV_PIX_FMT_RGBA128BE",
    "AV_PIX_FMT_RGBA128LE",
    "AV_PIX_FMT_RGBA64BE",
    "AV_PIX_FMT_RGBA64LE",
    "AV_PIX_FMT_RGBAF16BE",
    "AV_PIX_FMT_RGBAF16LE",
    "AV_PIX_FMT_RGBAF32BE",
    "AV_PIX_FMT_RGBAF32LE",
    "AV_PIX_FMT_RGBF16BE",
    "AV_PIX_FMT_RGBF16LE",
    "AV_PIX_FMT_RGBF32BE",
    "AV_PIX_FMT_RGBF32LE",
    "AV_PIX_FMT_UYVA",
    "AV_PIX_FMT_UYVY422",
    "AV_PIX_FMT_UYYVYY411",
    "AV_PIX_FMT_V30XBE",
    "AV_PIX_FMT_V30XLE",
    "AV_PIX_FMT_VAAPI",
    "AV_PIX_FMT_VDPAU",
    "AV_PIX_FMT_VIDEOTOOLBOX",
    "AV_PIX_FMT_VULKAN",
    "AV_PIX_FMT_VUYA",
    "AV_PIX_FMT_VUYX",
    "AV_PIX_FMT_VYU444",
    "AV_PIX_FMT_X2BGR10BE",
    "AV_PIX_FMT_X2BGR10LE",
    "AV_PIX_FMT_X2RGB10BE",
    "AV_PIX_FMT_X2RGB10LE",
    "AV_PIX_FMT_XV30BE",
    "AV_PIX_FMT_XV30LE",
    "AV_PIX_FMT_XV36BE",
    "AV_PIX_FMT_XV36LE",
    "AV_PIX_FMT_XV48BE",
    "AV_PIX_FMT_XV48LE",
    "AV_PIX_FMT_XYZ12BE",
    "AV_PIX_FMT_XYZ12LE",
    "AV_PIX_FMT_Y210BE",
    "AV_PIX_FMT_Y210LE",
    "AV_PIX_FMT_Y212BE",
    "AV_PIX_FMT_Y212LE",
    "AV_PIX_FMT_Y216BE",
    "AV_PIX_FMT_Y216LE",
    "AV_PIX_FMT_Y400A",
    "AV_PIX_FMT_YA16BE",
    "AV_PIX_FMT_YA16LE",
    "AV_PIX_FMT_YA8",
    "AV_PIX_FMT_YAF16BE",
    "AV_PIX_FMT_YAF16LE",
    "AV_PIX_FMT_YAF32BE",
    "AV_PIX_FMT_YAF32LE",
    "AV_PIX_FMT_YUV410P",
    "AV_PIX_FMT_YUV411P",
    "AV_PIX_FMT_YUV420P",
    "AV_PIX_FMT_YUV420P10BE",
    "AV_PIX_FMT_YUV420P10LE",
    "AV_PIX_FMT_YUV420P12BE",
    "AV_PIX_FMT_YUV420P12LE",
    "AV_PIX_FMT_YUV420P14BE",
    "AV_PIX_FMT_YUV420P14LE",
    "AV_PIX_FMT_YUV420P16BE",
    "AV_PIX_FMT_YUV420P16LE",
    "AV_PIX_FMT_YUV420P9BE",
    "AV_PIX_FMT_YUV420P9LE",
    "AV_PIX_FMT_YUV422P",
    "AV_PIX_FMT_YUV422P10BE",
    "AV_PIX_FMT_YUV422P10LE",
    "AV_PIX_FMT_YUV422P12BE",
    "AV_PIX_FMT_YUV422P12LE",
    "AV_PIX_FMT_YUV422P14BE",
    "AV_PIX_FMT_YUV422P14LE",
    "AV_PIX_FMT_YUV422P16BE",
    "AV_PIX_FMT_YUV422P16LE",
    "AV_PIX_FMT_YUV422P9BE",
    "AV_PIX_FMT_YUV422P9LE",
    "AV_PIX_FMT_YUV440P",
    "AV_PIX_FMT_YUV440P10BE",
    "AV_PIX_FMT_YUV440P10LE",
    "AV_PIX_FMT_YUV440P12BE",
    "AV_PIX_FMT_YUV440P12LE",
    "AV_PIX_FMT_YUV444P",
    "AV_PIX_FMT_YUV444P10BE",
    "AV_PIX_FMT_YUV444P10LE",
    "AV_PIX_FMT_YUV444P10MSBBE",
    "AV_PIX_FMT_YUV444P10MSBLE",
    "AV_PIX_FMT_YUV444P12BE",
    "AV_PIX_FMT_YUV444P12LE",
    "AV_PIX_FMT_YUV444P12MSBBE",
    "AV_PIX_FMT_YUV444P12MSBLE",
    "AV_PIX_FMT_YUV444P14BE",
    "AV_PIX_FMT_YUV444P14LE",
    "AV_PIX_FMT_YUV444P16BE",
    "AV_PIX_FMT_YUV444P16LE",
    "AV_PIX_FMT_YUV444P9BE",
    "AV_PIX_FMT_YUV444P9LE",
    "AV_PIX_FMT_YUVA420P",
    "AV_PIX_FMT_YUVA420P10BE",
    "AV_PIX_FMT_YUVA420P10LE",
    "AV_PIX_FMT_YUVA420P16BE",
    "AV_PIX_FMT_YUVA420P16LE",
    "AV_PIX_FMT_YUVA420P9BE",
    "AV_PIX_FMT_YUVA420P9LE",
    "AV_PIX_FMT_YUVA422P",
    "AV_PIX_FMT_YUVA422P10BE",
    "AV_PIX_FMT_YUVA422P10LE",
    "AV_PIX_FMT_YUVA422P12BE",
    "AV_PIX_FMT_YUVA422P12LE",
    "AV_PIX_FMT_YUVA422P16BE",
    "AV_PIX_FMT_YUVA422P16LE",
    "AV_PIX_FMT_YUVA422P9BE",
    "AV_PIX_FMT_YUVA422P9LE",
    "AV_PIX_FMT_YUVA444P",
    "AV_PIX_FMT_YUVA444P10BE",
    "AV_PIX_FMT_YUVA444P10LE",
    "AV_PIX_FMT_YUVA444P12BE",
    "AV_PIX_FMT_YUVA444P12LE",
    "AV_PIX_FMT_YUVA444P16BE",
    "AV_PIX_FMT_YUVA444P16LE",
]

# =========================================================================
# EMBEDDED DATA: FFmpeg audio/video filters comprehensive list
# =========================================================================
FFMPEG_FILTERS = [
    "aap", "abench", "acompressor", "acontrast", "acopy", "acrossfade", "acrossover",
    "acrusher", "acue", "addroi", "adeclick", "adeclip", "adecorrelate", "adelay",
    "adenorm", "aderivative", "adrc", "adynamicequalizer", "adynamicsmooth", "aecho",
    "aemphasis", "aeval", "aexciter", "afade", "afftdn", "afftfilt", "afir", "aformat",
    "afreqshift", "afwtdn", "agate", "aiir", "aintegral", "ainterleave", "alatency",
    "alimiter", "allpass", "aloop", "alphaextract", "alphamerge", "amerge", "ametadata",
    "amix", "amplify", "amultiply", "anequalizer", "anlmdn", "anlmf", "anlms", "anull",
    "apad", "aperms", "aphaser", "aphaseshift", "apsnr", "apsyclip", "apulsator",
    "arealtime", "aresample", "areverse", "arls", "arnndn", "asdr", "asegment", "aselect",
    "asendcmd", "asetnsamples", "asetpts", "asetrate", "asettb", "ashowinfo", "asidedata",
    "asisdr", "asoftclip", "aspectralstats", "asplit", "asr", "ass", "astats",
    "astreamselect", "asubboost", "asubcut", "asupercut", "asuperpass", "asuperstop",
    "atadenoise", "atempo", "atilt", "atrim", "avgblur", "avgblur_opencl", "avgblur_vulkan",
    "axcorrelate", "azmq", "backgroundkey", "bandpass", "bandreject", "bass", "bbox", "bench",
    "bilateral", "bilateral_cuda", "biquad", "bitplanenoise", "blackdetect",
    "blackdetect_vulkan", "blackframe", "blend", "blend_vulkan", "blockdetect", "blurdetect",
    "bm3d", "boxblur", "boxblur_opencl", "bs2b", "bwdif", "bwdif_cuda", "bwdif_vulkan", "cas",
    "ccrepack", "channelmap", "channelsplit", "chorus", "chromaber_vulkan", "chromahold",
    "chromakey", "chromakey_cuda", "chromanr", "chromashift", "ciescope", "codecview",
    "colorbalance", "colorchannelmixer", "colorcontrast", "colorcorrect", "colordetect",
    "colorhold", "colorize", "colorkey", "colorkey_opencl", "colorlevels", "colormap",
    "colormatrix", "colorspace", "colorspace_cuda", "colortemperature", "compand",
    "compensationdelay", "convolution", "convolution_opencl", "convolve", "copy", "coreimage",
    "corr", "cover_rect", "crop", "cropdetect", "crossfeed", "crystalizer", "cue", "curves",
    "datascope", "dblur", "dcshift", "dctdnoiz", "deband", "deblock", "decimate", "deconvolve",
    "dedot", "deesser", "deflate", "deflicker", "deinterlace_qsv", "deinterlace_vaapi",
    "dejudder", "delogo", "denoise_vaapi", "derain", "deshake", "deshake_opencl", "despill",
    "detelecine", "dialoguenhance", "dilation", "dilation_opencl", "displace", "dnn_classify",
    "dnn_detect", "dnn_processing", "doubleweave", "drawbox", "drawbox_vaapi", "drawgraph",
    "drawgrid", "drawtext", "drmeter", "dynaudnorm", "earwax", "ebur128", "edgedetect", "elbg",
    "entropy", "epx", "eq", "equalizer", "erosion", "erosion_opencl", "estdif", "exposure",
    "extractplanes", "extrastereo", "fade", "feedback", "fftdnoiz", "fftfilt", "field",
    "fieldhint", "fieldmatch", "fieldorder", "fillborders", "find_rect", "firequalizer",
    "flanger", "flip_vulkan", "floodfill", "format", "fps", "framepack", "framerate",
    "framestep", "freezedetect", "freezeframes", "frei0r", "fspp", "fsync", "gblur",
    "gblur_vulkan", "geq", "gradfun", "graphmonitor", "grayworld", "greyedge", "guided", "haas",
    "haldclut", "hdcd", "headphone", "hflip", "hflip_vulkan", "highpass", "highshelf", "histeq",
    "histogram", "hqdn3d", "hqx", "hstack", "hstack_qsv", "hstack_vaapi", "hsvhold", "hsvkey",
    "hue", "huesaturation", "hwdownload", "hwmap", "hwupload", "hwupload_cuda", "hysteresis",
    "iccdetect", "iccgen", "identity", "idet", "il", "inflate", "interlace", "interlace_vulkan",
    "interleave", "join", "kerndeint", "kirsch", "ladspa", "lagfun", "latency", "lcevc",
    "lenscorrection", "lensfun", "libplacebo", "libvmaf", "libvmaf_cuda", "limitdiff", "limiter",
    "loop", "loudnorm", "lowpass", "lowshelf", "lumakey", "lut", "lut1d", "lut2", "lut3d",
    "lutrgb", "lutyuv", "lv2", "maskedclamp", "maskedmax", "maskedmerge", "maskedmin",
    "maskedthreshold", "maskfun", "mcdeint", "mcompand", "median", "mergeplanes", "mestimate",
    "metadata", "midequalizer", "minterpolate", "mix", "monochrome", "morpho", "mpdecimate",
    "msad", "multiply", "negate", "nlmeans", "nlmeans_opencl", "nlmeans_vulkan", "nnedi",
    "noformat", "noise", "normalize", "null", "ocr", "ocv", "oscilloscope", "overlay",
    "overlay_cuda", "overlay_opencl", "overlay_qsv", "overlay_vaapi", "overlay_vulkan",
    "owdenoise", "pad", "pad_cuda", "pad_opencl", "pad_vaapi", "palettegen", "paletteuse",
    "pan", "perms", "perspective", "phase", "photosensitivity", "pixdesctest", "pixelize",
    "pixscope", "pp7", "premultiply", "prewitt", "prewitt_opencl", "procamp_vaapi",
    "program_opencl", "pseudocolor", "psnr", "pullup", "qp", "qrencode", "quirc", "random",
    "readeia608", "readvitc", "realtime", "remap", "remap_opencl", "removegrain", "removelogo",
    "repeatfields", "replaygain", "reverse", "rgbashift", "roberts", "roberts_opencl", "rotate",
    "rubberband", "sab", "scale", "scale2ref", "scale2ref_npp", "scale_cuda", "scale_d3d11",
    "scale_npp", "scale_qsv", "scale_vaapi", "scale_vt", "scale_vulkan", "scdet",
    "scdet_vulkan", "scharr", "scroll", "segment", "select", "selectivecolor", "sendcmd",
    "separatefields", "setdar", "setfield", "setparams", "setpts", "setrange", "setsar", "settb",
    "sharpen_npp", "sharpness_vaapi", "shear", "showinfo", "showpalette", "shuffleframes",
    "shufflepixels", "shuffleplanes", "sidechaincompress", "sidechaingate", "sidedata",
    "signalstats", "signature", "silencedetect", "silenceremove", "siti", "smartblur", "sobel",
    "sobel_opencl", "sofalizer", "speechnorm", "split", "spp", "sr", "sr_amf", "ssim", "ssim360",
    "stereo3d", "stereotools", "stereowiden", "streamselect", "subtitles", "super2xsai",
    "superequalizer", "surround", "swaprect", "swapuv", "tblend", "telecine", "thistogram",
    "threshold", "thumbnail", "thumbnail_cuda", "tile", "tiltandshift", "tiltshelf", "tinterlace",
    "tlut2", "tmedian", "tmidequalizer", "tmix", "tonemap", "tonemap_opencl", "tonemap_vaapi",
    "tpad", "transpose", "transpose_npp", "transpose_opencl", "transpose_vaapi", "transpose_vt",
    "transpose_vulkan", "treble", "tremolo", "trim", "unpremultiply", "unsharp", "unsharp_opencl",
    "untile", "uspp", "v360", "vaguedenoiser", "varblur", "vectorscope", "vflip", "vflip_vulkan",
    "vfrdet", "vibrance", "vibrato", "vidstabdetect", "vidstabtransform", "vif", "vignette",
    "virtualbass", "vmafmotion", "volume", "volumedetect", "vpp_amf", "vpp_qsv", "vstack",
    "vstack_qsv", "vstack_vaapi", "w3fdif", "waveform", "weave", "whisper", "xbr", "xcorrelate",
    "xfade", "xfade_opencl", "xfade_vulkan", "xmedian", "xpsnr", "xstack", "xstack_qsv",
    "xstack_vaapi", "yadif", "yadif_cuda", "yadif_videotoolbox", "yaepblur", "zmq", "zoompan",
    "zscale",
]

# =========================================================================
# Extract format mappings from embedded data
# =========================================================================
def load_ffmpeg_mlt_mapping():
    """Extract mappings from embedded data"""
    audio_map = FFMPEG_TO_MLT_MAPPINGS["audio_mapping"]
    video_map = FFMPEG_TO_MLT_MAPPINGS["video_mapping"]
    return audio_map, video_map


# =========================================================================
# Load filter list (embedded)
# =========================================================================
def load_filter_list():
    """Return embedded FFmpeg filter list"""
    return FFMPEG_FILTERS


# =========================================================================
# Build symbol -> source file map
# Parse allfilters.c for extern declarations to get symbol names,
# then find defining .c file.
# =========================================================================
def build_symbol_to_file_map():
    """Returns symbol_name -> filepath, e.g. 'ff_vf_colorbalance' -> '...vf_colorbalance.c'
    Also builds a map of all source file texts for macro-generated filter lookups."""
    sym_to_file = {}
    pattern = re.compile(r'const\s+FFFilter\s+(ff_\w+)\s*=')
    global _src_cache
    _src_cache = {}
    for fname in os.listdir(FFMPEG_DIR):
        if not fname.endswith('.c'):
            continue
        fpath = os.path.join(FFMPEG_DIR, fname)
        try:
            text = open(fpath).read()
        except Exception:
            continue
        _src_cache[fpath] = text
        for m in pattern.finditer(text):
            sym_to_file[m.group(1)] = fpath
    return sym_to_file


_src_cache = {}


def find_file_for_filter_name(filter_name):
    """Fallback: find source file where filter_name is defined via a DEFINE_*_FILTER macro
    or as .name = "filter_name" in an AVFilter struct."""
    # Match DEFINE_*FILTER(filter_name, ...) or .name = "filter_name"
    pattern = re.compile(
        r'DEFINE_\w+\s*\(\s*' + re.escape(filter_name) + r'\s*,'
        r'|\.name\s*=\s*"' + re.escape(filter_name) + r'"'
    )
    for fpath, text in _src_cache.items():
        if pattern.search(text):
            return fpath
    return None


# =========================================================================
# Parse allfilters.c to get filter_name -> symbol mapping
# =========================================================================
def parse_allfilters():
    """Returns filter_name -> symbol, e.g. 'colorbalance' -> 'ff_vf_colorbalance'"""
    name_to_sym = {}
    # extern declarations like: extern const FFFilter ff_vf_colorbalance;
    pattern = re.compile(r'extern\s+const\s+FFFilter\s+(ff_\w+)\s*;')
    text = open(ALLFILTERS).read()
    for m in pattern.finditer(text):
        sym = m.group(1)
        # strip prefix ff_Xt_ where Xt is 2-3 char type code
        # e.g. ff_vf_colorbalance -> colorbalance
        # ff_af_acompressor -> acompressor
        # ff_avf_concat -> concat
        # ff_src_color -> color
        # ff_asrc_anullsrc -> anullsrc
        # ff_bsf_... -> skip (bitstream filters, not in our list)
        parts = sym.split('_', 2)  # ['ff', 'vf', 'colorbalance']
        if len(parts) >= 3:
            name = parts[2]
        else:
            name = sym[3:]  # fallback
        # Handle multi-segment names: ff_ff_filter -> keep as-is
        name_to_sym[name] = sym
    return name_to_sym


# =========================================================================
# Extract format lists from a source file
# Handles:
#   static const enum AVPixelFormat pix_fmts[] = { ... };
#   static const enum AVSampleFormat sample_fmts[] = { ... };
#   FILTER_PIXFMTS(AV_PIX_FMT_X, ...)
#   FILTER_SAMPLEFMTS(AV_SAMPLE_FMT_X, ...)
#   FILTER_PIXFMTS_ARRAY(pix_fmts)  -> resolves to the array
#   FILTER_SAMPLEFMTS_ARRAY(sample_fmts) -> resolves to the array
# Returns (pix_fmts: list[str], sample_fmts: list[str]) with raw FFmpeg names
# =========================================================================


def extract_raw_brace_block(text, start_pos):
    """Extract the raw text content of a { ... } block starting at start_pos."""
    depth = 0
    i = start_pos
    buf = []
    while i < len(text):
        c = text[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                break
        elif depth > 0:
            buf.append(c)
        i += 1
    return ''.join(buf)


def extract_enum_values_from_brace_block(text, start_pos):
    """Extract comma-separated C enum values from a { ... } block starting at start_pos."""
    block = extract_raw_brace_block(text, start_pos)
    return re.findall(r'AV_(?:PIX_FMT|SAMPLE_FMT)_\w+', block)


def is_metadata_only_filter(filepath, filter_name):
    """
    Check if a filter is flagged with AVFILTER_FLAG_METADATA_ONLY.
    This means the filter only modifies metadata, not pixel data.
    """
    try:
        text = open(filepath).read()
    except Exception:
        return False

    # Look for AVFILTER_FLAG_METADATA_ONLY in the FFFilter struct for this filter
    # Match patterns like: const FFFilter ff_af_acue = { ... .p.flags = AVFILTER_FLAG_METADATA_ONLY ... }
    pattern = re.compile(
        r'const\s+FFFilter\s+ff_\w+\s*=\s*\{[^}]*\.p\.flags\s*=\s*AVFILTER_FLAG_METADATA_ONLY'
    )
    if pattern.search(text):
        return True
    return False


def extract_formats_from_file(filepath):
    """
    Returns (pix_fmt_set, sample_fmt_set) or (None, None) if formats are dynamic.
    An empty list means 'no static formats found'.
    None means 'dynamic / accepts all' - leave blank.
    """
    try:
        text = open(filepath).read()
    except Exception:
        return None, None

    # --- Step 0: Build a macro expansion map for macros containing AV_*_FMT_* ---
    macro_expansions = {}  # macro_name -> list of AV_*_FMT_* values
    # Match #define NAME \ followed by lines with AV_PIX_FMT_* or AV_SAMPLE_FMT_*
    macro_re = re.compile(
        r'#\s*define\s+(\w+)\s+((?:[^\n\\]*\\\n)*[^\n\\]*)',
        re.MULTILINE
    )
    for m in macro_re.finditer(text):
        name = m.group(1)
        body = m.group(2).replace('\\\n', ' ')
        fmts = re.findall(r'AV_(?:PIX_FMT|SAMPLE_FMT)_\w+', body)
        if fmts:
            macro_expansions[name] = fmts

    def expand_block(block):
        """Extract AV_*_FMT_* values from a block, expanding known macros."""
        results = re.findall(r'AV_(?:PIX_FMT|SAMPLE_FMT)_\w+', block)
        # Also expand any macro names that appear in the block
        for word in re.findall(r'\b([A-Z][A-Z0-9_]+)\b', block):
            if word in macro_expansions:
                results.extend(macro_expansions[word])
        return results

    pix_fmt_enums = []
    sample_fmt_enums = []

    # Match static pix_fmts arrays, including multidimensional declarations
    # like: sample_fmts[3][3] = { ... } used by filters such as aap.
    for m in re.finditer(
        r'static\s+const\s+enum\s+AVPixelFormat\s+\w+(?:\s*\[[^\]]*\])+\s*=\s*\{', text
    ):
        vals = expand_block(
            extract_raw_brace_block(text, m.end() - 1)
        )
        pix_fmt_enums.extend(vals)

    # Match static sample_fmts arrays, including multidimensional declarations.
    for m in re.finditer(
        r'static\s+const\s+enum\s+AVSampleFormat\s+\w+(?:\s*\[[^\]]*\])+\s*=\s*\{', text
    ):
        vals = expand_block(
            extract_raw_brace_block(text, m.end() - 1)
        )
        sample_fmt_enums.extend(vals)

    # FILTER_PIXFMTS(...) macro - inline list
    for m in re.finditer(r'FILTER_PIXFMTS\s*\(', text):
        i = m.end()
        depth = 1
        buf = []
        while i < len(text) and depth > 0:
            if text[i] == '(': depth += 1
            elif text[i] == ')': depth -= 1
            if depth > 0: buf.append(text[i])
            i += 1
        pix_fmt_enums.extend(expand_block(''.join(buf)))

    # FILTER_SINGLE_PIXFMT(...) macro - one explicit pixel format
    for m in re.finditer(r'FILTER_SINGLE_PIXFMT\s*\(', text):
        i = m.end()
        depth = 1
        buf = []
        while i < len(text) and depth > 0:
            if text[i] == '(':
                depth += 1
            elif text[i] == ')':
                depth -= 1
            if depth > 0:
                buf.append(text[i])
            i += 1
        pix_fmt_enums.extend(expand_block(''.join(buf)))

    # FILTER_SAMPLEFMTS(...) macro - inline list
    for m in re.finditer(r'FILTER_SAMPLEFMTS\s*\(', text):
        i = m.end()
        depth = 1
        buf = []
        while i < len(text) and depth > 0:
            if text[i] == '(': depth += 1
            elif text[i] == ')': depth -= 1
            if depth > 0: buf.append(text[i])
            i += 1
        sample_fmt_enums.extend(expand_block(''.join(buf)))

    # FILTER_SINGLE_SAMPLEFMT(...) macro - one explicit sample format
    for m in re.finditer(r'FILTER_SINGLE_SAMPLEFMT\s*\(', text):
        i = m.end()
        depth = 1
        buf = []
        while i < len(text) and depth > 0:
            if text[i] == '(':
                depth += 1
            elif text[i] == ')':
                depth -= 1
            if depth > 0:
                buf.append(text[i])
            i += 1
        sample_fmt_enums.extend(expand_block(''.join(buf)))

    # Dynamic query_formats - if present and no static formats found, signal dynamic
    has_dynamic = bool(re.search(r'FILTER_QUERY_FUNC|query_formats\s*\(', text))
    has_all_formats = bool(re.search(r'ff_all_formats\b|ff_all_samplefmts\b', text))

    pix_fmts = None
    sample_fmts = None

    if pix_fmt_enums:
        pix_fmts = list(dict.fromkeys(pix_fmt_enums))
    elif has_all_formats or (has_dynamic and not sample_fmt_enums):
        pix_fmts = None

    if sample_fmt_enums:
        sample_fmts = list(dict.fromkeys(sample_fmt_enums))
    elif has_all_formats or (has_dynamic and not pix_fmt_enums):
        sample_fmts = None

    return pix_fmts, sample_fmts


# =========================================================================
# Map FFmpeg format enum to MLT format name
# =========================================================================
def ffmpeg_fmt_to_key(fmt_enum):
    """Convert AV_PIX_FMT_YUV420P -> yuv420p, AV_SAMPLE_FMT_FLT -> flt"""
    if fmt_enum.startswith('AV_PIX_FMT_'):
        return fmt_enum[len('AV_PIX_FMT_'):].lower()
    if fmt_enum.startswith('AV_SAMPLE_FMT_'):
        return fmt_enum[len('AV_SAMPLE_FMT_'):].lower()
    return fmt_enum.lower()


def map_formats(ffmpeg_enums, mapping):
    """Map a list of AV_*_FMT_* names to deduplicated MLT format names.
    Handles native-endian aliases (e.g. AV_PIX_FMT_RGB48 -> try 'rgb48be' fallback)."""
    mlt_fmts = []
    for e in ffmpeg_enums:
        key = ffmpeg_fmt_to_key(e)
        mlt = mapping.get(key)
        # Fallback: try with 'be'/'le' suffix for native-endian aliases
        if not mlt:
            mlt = mapping.get(key + 'be') or mapping.get(key + 'le')
        if mlt and mlt != 'none' and mlt not in mlt_fmts:
            mlt_fmts.append(mlt)
    return mlt_fmts


# =========================================================================
# Check if a filter is likely hardware-accelerated based on name pattern
# =========================================================================
def is_likely_hardware_filter(filter_name):
    """Heuristic: check if filter name suggests hardware acceleration."""
    hw_patterns = ['_qsv', '_vaapi', '_videotoolbox', '_cuda', '_opencl', '_vulkan', 'coreimage']
    return any(pattern in filter_name for pattern in hw_patterns)


# =========================================================================
# Main
# =========================================================================
def main():
    global FFMPEG_DIR, ALLFILTERS, OUT_YML
    
    # Parse command-line arguments
    args = parse_args()
    
    print(f"Using FFmpeg directory: {FFMPEG_DIR}")
    print(f"Output YAML file: {OUT_YML}")
    
    audio_map, video_map = load_ffmpeg_mlt_mapping()
    filter_names = load_filter_list()
    print(f"Loaded {len(filter_names)} filters, {len(audio_map)} audio mappings, {len(video_map)} video mappings")

    name_to_sym = parse_allfilters()
    sym_to_file = build_symbol_to_file_map()
    print(f"Parsed {len(name_to_sym)} filter symbols, {len(sym_to_file)} source files")

    results = {}  # filter_name -> {'audio_formats': [...], 'image_formats': [...], 'metadata_only': bool, 'format_agnostic': bool, 'hardware_only': bool}
    stats = {'static_pix': 0, 'static_samp': 0, 'dynamic': 0, 'not_found': 0, 'metadata_only': 0, 'format_agnostic': 0, 'hardware_only': 0}

    for name in filter_names:
        sym = name_to_sym.get(name)
        src_file = sym_to_file.get(sym) if sym else None

        if not src_file:
            # Fallback for macro-generated filters (e.g. biquad family, lut family)
            src_file = find_file_for_filter_name(name)

        if not src_file:
            # Could not find source file - check if it's likely hardware-accelerated
            # based on naming pattern
            entry = {'metadata_only': False, 'format_agnostic': False, 'hardware_only': False}
            if is_likely_hardware_filter(name):
                entry['hardware_only'] = True
                stats['hardware_only'] += 1
            else:
                stats['not_found'] += 1
            results[name] = entry
            continue

        # Check if this filter is metadata-only
        is_metadata_only = is_metadata_only_filter(src_file, name)

        entry = {'metadata_only': is_metadata_only, 'format_agnostic': False, 'hardware_only': False}
        if is_metadata_only:
            stats['metadata_only'] += 1
        else:
            pix_enums, samp_enums = extract_formats_from_file(src_file)

            # If both audio and video format enums are None (not empty lists, but None),
            # it means the filter accepts all formats dynamically - mark as format_agnostic
            if pix_enums is None and samp_enums is None:
                entry['format_agnostic'] = True
                stats['format_agnostic'] += 1
            else:
                # Try to map the extracted formats
                mlt_img = map_formats(pix_enums, video_map) if pix_enums is not None else []
                mlt_aud = map_formats(samp_enums, audio_map) if samp_enums is not None else []

                # Check if we extracted formats but couldn't map any of them
                # This indicates hardware-only or unsupported formats
                has_extracted_pix = pix_enums is not None and len(pix_enums) > 0
                has_extracted_samp = samp_enums is not None and len(samp_enums) > 0
                has_mapped_pix = len(mlt_img) > 0
                has_mapped_samp = len(mlt_aud) > 0

                # If we extracted formats but couldn't map them, it's hardware-only
                if (has_extracted_pix and not has_mapped_pix) or (has_extracted_samp and not has_mapped_samp):
                    entry['hardware_only'] = True
                    stats['hardware_only'] += 1
                else:
                    # Normal case: add whatever we could map
                    if mlt_img:
                        entry['image_formats'] = mlt_img
                        stats['static_pix'] += 1
                    if mlt_aud:
                        entry['audio_formats'] = mlt_aud
                        stats['static_samp'] += 1

        results[name] = entry

    print(f"Stats: static_pix={stats['static_pix']}, static_samp={stats['static_samp']}, "
          f"dynamic/all={stats['dynamic']}, metadata_only={stats['metadata_only']}, "
          f"format_agnostic={stats['format_agnostic']}, hardware_only={stats['hardware_only']}, not_found={stats['not_found']}")
    print(f"Filters with image_formats: {sum(1 for v in results.values() if 'image_formats' in v)}")
    print(f"Filters with audio_formats: {sum(1 for v in results.values() if 'audio_formats' in v)}")

    # Write YAML (manually, to avoid dependency on PyYAML)
    lines = [
        "# FFmpeg filter supported MLT formats",
        "# One entry per filter. audio_formats/image_formats populated from FFmpeg source.",
        "# Filters without known formats are placeholders encoded as \"\" (not empty maps).",
        "# Filters marked as 'metadata only' process metadata only, passing data unchanged.",
        "# Filters marked as 'format agnostic' accept all formats dynamically with no static restrictions.",
        "# Filters marked as 'hardware only' accept only hardware/GPU formats not directly supported by MLT.",
        "",
    ]
    for name in filter_names:
        entry = results.get(name, {})
        is_metadata_only = entry.get('metadata_only', False)
        is_format_agnostic = entry.get('format_agnostic', False)
        is_hardware_only = entry.get('hardware_only', False)

        if not entry or (is_metadata_only and 'audio_formats' not in entry and 'image_formats' not in entry) \
           or (is_format_agnostic and 'audio_formats' not in entry and 'image_formats' not in entry) \
           or (is_hardware_only and 'audio_formats' not in entry and 'image_formats' not in entry):
            if is_metadata_only:
                lines.append(f'{name}: ""  # metadata only')
            elif is_hardware_only:
                lines.append(f'{name}: ""  # hardware only')
            elif is_format_agnostic:
                lines.append(f'{name}: ""  # format agnostic')
            else:
                lines.append(f'{name}: ""')
        else:
            lines.append(f'{name}:')
            if 'audio_formats' in entry:
                lines.append('  audio_formats:')
                for fmt in entry['audio_formats']:
                    lines.append(f'    - {fmt}')
            if 'image_formats' in entry:
                lines.append('  image_formats:')
                for fmt in entry['image_formats']:
                    lines.append(f'    - {fmt}')
            if is_metadata_only:
                lines.append('  # metadata only')
            elif is_hardware_only:
                lines.append('  # hardware only')
            elif is_format_agnostic:
                lines.append('  # format agnostic')

    with open(OUT_YML, 'w') as f:
        f.write('\n'.join(lines) + '\n')

    print(f"Written: {OUT_YML}")


if __name__ == '__main__':
    main()
