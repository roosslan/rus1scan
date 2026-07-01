/****************************************************************************
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
*
*  File: wiadef.h
*
*  Version: 4.0
*
*  Description: WIA constant definitions
*
*****************************************************************************/

#if (_WIN32_WINNT >= 0x0501) // Windows XP and later

#pragma once

#ifndef _WIADEF_
#define _WIADEF_

#include <winapifamily.h>

#include <pshpack8.h>
#ifndef _NO_COM
#include <objbase.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

//
// WIA property ID and string constants
//

#define WIA_DIP_DEV_ID                                       2 // 0x2
#define WIA_DIP_DEV_ID_STR                                   L"Unique Device ID"

#define WIA_DIP_VEND_DESC                                    3 // 0x3
#define WIA_DIP_VEND_DESC_STR                                L"Manufacturer"

#define WIA_DIP_DEV_DESC                                     4 // 0x4
#define WIA_DIP_DEV_DESC_STR                                 L"Description"

#define WIA_DIP_DEV_TYPE                                     5 // 0x5
#define WIA_DIP_DEV_TYPE_STR                                 L"Type"

#define WIA_DIP_PORT_NAME                                    6 // 0x6
#define WIA_DIP_PORT_NAME_STR                                L"Port"

#define WIA_DIP_DEV_NAME                                     7 // 0x7
#define WIA_DIP_DEV_NAME_STR                                 L"Name"

#define WIA_DIP_SERVER_NAME                                  8 // 0x8
#define WIA_DIP_SERVER_NAME_STR                              L"Server"

#define WIA_DIP_REMOTE_DEV_ID                                9 // 0x9
#define WIA_DIP_REMOTE_DEV_ID_STR                            L"Remote Device ID"

#define WIA_DIP_UI_CLSID                                     10 // 0xa
#define WIA_DIP_UI_CLSID_STR                                 L"UI Class ID"

#define WIA_DIP_HW_CONFIG                                    11 // 0xb
#define WIA_DIP_HW_CONFIG_STR                                L"Hardware Configuration"

#define WIA_DIP_BAUDRATE                                     12 // 0xc
#define WIA_DIP_BAUDRATE_STR                                 L"BaudRate"

#define WIA_DIP_STI_GEN_CAPABILITIES                         13 // 0xd
#define WIA_DIP_STI_GEN_CAPABILITIES_STR                     L"STI Generic Capabilities"

#define WIA_DIP_WIA_VERSION                                  14 // 0xe
#define WIA_DIP_WIA_VERSION_STR                              L"WIA Version"

#define WIA_DIP_DRIVER_VERSION                               15 // 0xf
#define WIA_DIP_DRIVER_VERSION_STR                           L"Driver Version"

#define WIA_DIP_PNP_ID                                       16 // 0x10
#define WIA_DIP_PNP_ID_STR                                   L"PnP ID String"

#define WIA_DIP_STI_DRIVER_VERSION                           17 // 0x11
#define WIA_DIP_STI_DRIVER_VERSION_STR                       L"STI Driver Version"

#define WIA_DPA_FIRMWARE_VERSION                             1026 // 0x402
#define WIA_DPA_FIRMWARE_VERSION_STR                         L"Firmware Version"

#define WIA_DPA_CONNECT_STATUS                               1027 // 0x403
#define WIA_DPA_CONNECT_STATUS_STR                           L"Connect Status"

#define WIA_DPA_DEVICE_TIME                                  1028 // 0x404
#define WIA_DPA_DEVICE_TIME_STR                              L"Device Time"

#define WIA_DPC_PICTURES_TAKEN                               2050 // 0x802
#define WIA_DPC_PICTURES_TAKEN_STR                           L"Pictures Taken"

#define WIA_DPC_PICTURES_REMAINING                           2051 // 0x803
#define WIA_DPC_PICTURES_REMAINING_STR                       L"Pictures Remaining"

#define WIA_DPC_EXPOSURE_MODE                                2052 // 0x804
#define WIA_DPC_EXPOSURE_MODE_STR                            L"Exposure Mode"

#define WIA_DPC_EXPOSURE_COMP                                2053 // 0x805
#define WIA_DPC_EXPOSURE_COMP_STR                            L"Exposure Compensation"

#define WIA_DPC_EXPOSURE_TIME                                2054 // 0x806
#define WIA_DPC_EXPOSURE_TIME_STR                            L"Exposure Time"

#define WIA_DPC_FNUMBER                                      2055 // 0x807
#define WIA_DPC_FNUMBER_STR                                  L"F Number"

#define WIA_DPC_FLASH_MODE                                   2056 // 0x808
#define WIA_DPC_FLASH_MODE_STR                               L"Flash Mode"

#define WIA_DPC_FOCUS_MODE                                   2057 // 0x809
#define WIA_DPC_FOCUS_MODE_STR                               L"Focus Mode"

#define WIA_DPC_FOCUS_MANUAL_DIST                            2058 // 0x80a
#define WIA_DPC_FOCUS_MANUAL_DIST_STR                        L"Focus Manual Dist"

#define WIA_DPC_ZOOM_POSITION                                2059 // 0x80b
#define WIA_DPC_ZOOM_POSITION_STR                            L"Zoom Position"

#define WIA_DPC_PAN_POSITION                                 2060 // 0x80c
#define WIA_DPC_PAN_POSITION_STR                             L"Pan Position"

#define WIA_DPC_TILT_POSITION                                2061 // 0x80d
#define WIA_DPC_TILT_POSITION_STR                            L"Tilt Position"

#define WIA_DPC_TIMER_MODE                                   2062 // 0x80e
#define WIA_DPC_TIMER_MODE_STR                               L"Timer Mode"

#define WIA_DPC_TIMER_VALUE                                  2063 // 0x80f
#define WIA_DPC_TIMER_VALUE_STR                              L"Timer Value"

#define WIA_DPC_POWER_MODE                                   2064 // 0x810
#define WIA_DPC_POWER_MODE_STR                               L"Power Mode"

#define WIA_DPC_BATTERY_STATUS                               2065 // 0x811
#define WIA_DPC_BATTERY_STATUS_STR                           L"Battery Status"

#define WIA_DPC_THUMB_WIDTH                                  2066 // 0x812
#define WIA_DPC_THUMB_WIDTH_STR                              L"Thumbnail Width"

#define WIA_DPC_THUMB_HEIGHT                                 2067 // 0x813
#define WIA_DPC_THUMB_HEIGHT_STR                             L"Thumbnail Height"

#define WIA_DPC_PICT_WIDTH                                   2068 // 0x814
#define WIA_DPC_PICT_WIDTH_STR                               L"Picture Width"

#define WIA_DPC_PICT_HEIGHT                                  2069 // 0x815
#define WIA_DPC_PICT_HEIGHT_STR                              L"Picture Height"

#define WIA_DPC_DIMENSION                                    2070 // 0x816
#define WIA_DPC_DIMENSION_STR                                L"Dimension"

#define WIA_DPC_COMPRESSION_SETTING                          2071 // 0x817
#define WIA_DPC_COMPRESSION_SETTING_STR                      L"Compression Setting"

#define WIA_DPC_FOCUS_METERING                               2072 // 0x818
#define WIA_DPC_FOCUS_METERING_STR                           L"Focus Metering Mode"

#define WIA_DPC_TIMELAPSE_INTERVAL                           2073 // 0x819
#define WIA_DPC_TIMELAPSE_INTERVAL_STR                       L"Timelapse Interval"

#define WIA_DPC_TIMELAPSE_NUMBER                             2074 // 0x81a
#define WIA_DPC_TIMELAPSE_NUMBER_STR                         L"Timelapse Number"

#define WIA_DPC_BURST_INTERVAL                               2075 // 0x81b
#define WIA_DPC_BURST_INTERVAL_STR                           L"Burst Interval"

#define WIA_DPC_BURST_NUMBER                                 2076 // 0x81c
#define WIA_DPC_BURST_NUMBER_STR                             L"Burst Number"

#define WIA_DPC_EFFECT_MODE                                  2077 // 0x81d
#define WIA_DPC_EFFECT_MODE_STR                              L"Effect Mode"

#define WIA_DPC_DIGITAL_ZOOM                                 2078 // 0x81e
#define WIA_DPC_DIGITAL_ZOOM_STR                             L"Digital Zoom"

#define WIA_DPC_SHARPNESS                                    2079 // 0x81f
#define WIA_DPC_SHARPNESS_STR                                L"Sharpness"

#define WIA_DPC_CONTRAST                                     2080 // 0x820
#define WIA_DPC_CONTRAST_STR                                 L"Contrast"

#define WIA_DPC_CAPTURE_MODE                                 2081 // 0x821
#define WIA_DPC_CAPTURE_MODE_STR                             L"Capture Mode"

#define WIA_DPC_CAPTURE_DELAY                                2082 // 0x822
#define WIA_DPC_CAPTURE_DELAY_STR                            L"Capture Delay"

#define WIA_DPC_EXPOSURE_INDEX                               2083 // 0x823
#define WIA_DPC_EXPOSURE_INDEX_STR                           L"Exposure Index"

#define WIA_DPC_EXPOSURE_METERING_MODE                       2084 // 0x824
#define WIA_DPC_EXPOSURE_METERING_MODE_STR                   L"Exposure Metering Mode"

#define WIA_DPC_FOCUS_METERING_MODE                          2085 // 0x825
#define WIA_DPC_FOCUS_METERING_MODE_STR                      L"Focus Metering Mode"

#define WIA_DPC_FOCUS_DISTANCE                               2086 // 0x826
#define WIA_DPC_FOCUS_DISTANCE_STR                           L"Focus Distance"

#define WIA_DPC_FOCAL_LENGTH                                 2087 // 0x827
#define WIA_DPC_FOCAL_LENGTH_STR                             L"Focus Length"

#define WIA_DPC_RGB_GAIN                                     2088 // 0x828
#define WIA_DPC_RGB_GAIN_STR                                 L"RGB Gain"

#define WIA_DPC_WHITE_BALANCE                                2089 // 0x829
#define WIA_DPC_WHITE_BALANCE_STR                            L"White Balance"

#define WIA_DPC_UPLOAD_URL                                   2090 // 0x82a
#define WIA_DPC_UPLOAD_URL_STR                               L"Upload URL"

#define WIA_DPC_ARTIST                                       2091 // 0x82b
#define WIA_DPC_ARTIST_STR                                   L"Artist"

#define WIA_DPC_COPYRIGHT_INFO                               2092 // 0x82c
#define WIA_DPC_COPYRIGHT_INFO_STR                           L"Copyright Info"

#define WIA_DPS_HORIZONTAL_BED_SIZE                          3074 // 0xc02
#define WIA_DPS_HORIZONTAL_BED_SIZE_STR                      L"Horizontal Bed Size"

#define WIA_DPS_VERTICAL_BED_SIZE                            3075 // 0xc03
#define WIA_DPS_VERTICAL_BED_SIZE_STR                        L"Vertical Bed Size"

#define WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE                   3076 // 0xc04
#define WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE_STR               L"Horizontal Sheet Feed Size"

#define WIA_DPS_VERTICAL_SHEET_FEED_SIZE                     3077 // 0xc05
#define WIA_DPS_VERTICAL_SHEET_FEED_SIZE_STR                 L"Vertical Sheet Feed Size"

#define WIA_DPS_SHEET_FEEDER_REGISTRATION                    3078 // 0xc06
#define WIA_DPS_SHEET_FEEDER_REGISTRATION_STR                L"Sheet Feeder Registration"

#define WIA_DPS_HORIZONTAL_BED_REGISTRATION                  3079 // 0xc07
#define WIA_DPS_HORIZONTAL_BED_REGISTRATION_STR              L"Horizontal Bed Registration"

#define WIA_DPS_VERTICAL_BED_REGISTRATION                    3080 // 0xc08
#define WIA_DPS_VERTICAL_BED_REGISTRATION_STR                L"Vertical Bed Registration"

#define WIA_DPS_PLATEN_COLOR                                 3081 // 0xc09
#define WIA_DPS_PLATEN_COLOR_STR                             L"Platen Color"

#define WIA_DPS_PAD_COLOR                                    3082 // 0xc0a
#define WIA_DPS_PAD_COLOR_STR                                L"Pad Color"

#define WIA_DPS_FILTER_SELECT                                3083 // 0xc0b
#define WIA_DPS_FILTER_SELECT_STR                            L"Filter Select"

#define WIA_DPS_DITHER_SELECT                                3084 // 0xc0c
#define WIA_DPS_DITHER_SELECT_STR                            L"Dither Select"

#define WIA_DPS_DITHER_PATTERN_DATA                          3085 // 0xc0d
#define WIA_DPS_DITHER_PATTERN_DATA_STR                      L"Dither Pattern Data"

#define WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES               3086 // 0xc0e
#define WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR           L"Document Handling Capabilities"

#define WIA_DPS_DOCUMENT_HANDLING_STATUS                     3087 // 0xc0f
#define WIA_DPS_DOCUMENT_HANDLING_STATUS_STR                 L"Document Handling Status"

#define WIA_DPS_DOCUMENT_HANDLING_SELECT                     3088 // 0xc10
#define WIA_DPS_DOCUMENT_HANDLING_SELECT_STR                 L"Document Handling Select"

#define WIA_DPS_DOCUMENT_HANDLING_CAPACITY                   3089 // 0xc11
#define WIA_DPS_DOCUMENT_HANDLING_CAPACITY_STR               L"Document Handling Capacity"

#define WIA_DPS_OPTICAL_XRES                                 3090 // 0xc12
#define WIA_DPS_OPTICAL_XRES_STR                             L"Horizontal Optical Resolution"

#define WIA_DPS_OPTICAL_YRES                                 3091 // 0xc13
#define WIA_DPS_OPTICAL_YRES_STR                             L"Vertical Optical Resolution"

#define WIA_DPS_ENDORSER_CHARACTERS                          3092 // 0xc14, superseded by WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS
#define WIA_DPS_ENDORSER_CHARACTERS_STR                      L"Endorser Characters"

#define WIA_DPS_ENDORSER_STRING                              3093 // 0xc15, superseded by WIA_IPS_PRINTER_ENDORSER_STRING
#define WIA_DPS_ENDORSER_STRING_STR                          L"Endorser String"

#define WIA_DPS_SCAN_AHEAD_PAGES                             3094 // 0xc16, superseded by WIA_IPS_SCAN_AHEAD
#define WIA_DPS_SCAN_AHEAD_PAGES_STR                         L"Scan Ahead Pages"

#define WIA_DPS_MAX_SCAN_TIME                                3095 // 0xc17
#define WIA_DPS_MAX_SCAN_TIME_STR                            L"Max Scan Time"

#define WIA_DPS_PAGES                                        3096 // 0xc18
#define WIA_DPS_PAGES_STR                                    L"Pages"

#define WIA_DPS_PAGE_SIZE                                    3097 // 0xc19
#define WIA_DPS_PAGE_SIZE_STR                                L"Page Size"

#define WIA_DPS_PAGE_WIDTH                                   3098 // 0xc1a
#define WIA_DPS_PAGE_WIDTH_STR                               L"Page Width"

#define WIA_DPS_PAGE_HEIGHT                                  3099 // 0xc1b
#define WIA_DPS_PAGE_HEIGHT_STR                              L"Page Height"

#define WIA_DPS_PREVIEW                                      3100 // 0xc1c
#define WIA_DPS_PREVIEW_STR                                  L"Preview"

#define WIA_DPS_TRANSPARENCY                                 3101 // 0xc1d
#define WIA_DPS_TRANSPARENCY_STR                             L"Transparency Adapter"

#define WIA_DPS_TRANSPARENCY_SELECT                          3102 // 0xc1e
#define WIA_DPS_TRANSPARENCY_SELECT_STR                      L"Transparency Adapter Select"

#define WIA_DPS_SHOW_PREVIEW_CONTROL                         3103 // 0xc1f
#define WIA_DPS_SHOW_PREVIEW_CONTROL_STR                     L"Show preview control"

#define WIA_DPS_MIN_HORIZONTAL_SHEET_FEED_SIZE               3104 // 0xc20
#define WIA_DPS_MIN_HORIZONTAL_SHEET_FEED_SIZE_STR           L"Minimum Horizontal Sheet Feed Size"

#define WIA_DPS_MIN_VERTICAL_SHEET_FEED_SIZE                 3105 // 0xc21
#define WIA_DPS_MIN_VERTICAL_SHEET_FEED_SIZE_STR             L"Minimum Vertical Sheet Feed Size"

#define WIA_DPS_TRANSPARENCY_CAPABILITIES                    3106 // 0xc22
#define WIA_DPS_TRANSPARENCY_CAPABILITIES_STR                L"Transparency Adapter Capabilities"

#define WIA_DPS_TRANSPARENCY_STATUS                          3107 // 0xc23
#define WIA_DPS_TRANSPARENCY_STATUS_STR                      L"Transparency Adapter Status"

#define WIA_DPF_MOUNT_POINT                                  3330 // 0xd02
#define WIA_DPF_MOUNT_POINT_STR                              L"Directory mount point"

#define WIA_DPV_LAST_PICTURE_TAKEN                           3586 // 0xe02
#define WIA_DPV_LAST_PICTURE_TAKEN_STR                       L"Last Picture Taken"

#define WIA_DPV_IMAGES_DIRECTORY                             3587 // 0xe03
#define WIA_DPV_IMAGES_DIRECTORY_STR                         L"Images Directory"

#define WIA_DPV_DSHOW_DEVICE_PATH                            3588 // 0xe04
#define WIA_DPV_DSHOW_DEVICE_PATH_STR                        L"Directshow Device Path"

#define WIA_IPA_ITEM_NAME                                    4098 // 0x1002
#define WIA_IPA_ITEM_NAME_STR                                L"Item Name"

#define WIA_IPA_FULL_ITEM_NAME                               4099 // 0x1003
#define WIA_IPA_FULL_ITEM_NAME_STR                           L"Full Item Name"

#define WIA_IPA_ITEM_TIME                                    4100 // 0x1004
#define WIA_IPA_ITEM_TIME_STR                                L"Item Time Stamp"

#define WIA_IPA_ITEM_FLAGS                                   4101 // 0x1005
#define WIA_IPA_ITEM_FLAGS_STR                               L"Item Flags"

#define WIA_IPA_ACCESS_RIGHTS                                4102 // 0x1006
#define WIA_IPA_ACCESS_RIGHTS_STR                            L"Access Rights"

#define WIA_IPA_DATATYPE                                     4103 // 0x1007
#define WIA_IPA_DATATYPE_STR                                 L"Data Type"

#define WIA_IPA_DEPTH                                        4104 // 0x1008
#define WIA_IPA_DEPTH_STR                                    L"Bits Per Pixel"

#define WIA_IPA_PREFERRED_FORMAT                             4105 // 0x1009
#define WIA_IPA_PREFERRED_FORMAT_STR                         L"Preferred Format"

#define WIA_IPA_FORMAT                                       4106 // 0x100a
#define WIA_IPA_FORMAT_STR                                   L"Format"

#define WIA_IPA_COMPRESSION                                  4107 // 0x100b
#define WIA_IPA_COMPRESSION_STR                              L"Compression"

#define WIA_IPA_TYMED                                        4108 // 0x100c
#define WIA_IPA_TYMED_STR                                    L"Media Type"

#define WIA_IPA_CHANNELS_PER_PIXEL                           4109 // 0x100d
#define WIA_IPA_CHANNELS_PER_PIXEL_STR                       L"Channels Per Pixel"

#define WIA_IPA_BITS_PER_CHANNEL                             4110 // 0x100e
#define WIA_IPA_BITS_PER_CHANNEL_STR                         L"Bits Per Channel"

#define WIA_IPA_PLANAR                                       4111 // 0x100f
#define WIA_IPA_PLANAR_STR                                   L"Planar"

#define WIA_IPA_PIXELS_PER_LINE                              4112 // 0x1010
#define WIA_IPA_PIXELS_PER_LINE_STR                          L"Pixels Per Line"

#define WIA_IPA_BYTES_PER_LINE                               4113 // 0x1011
#define WIA_IPA_BYTES_PER_LINE_STR                           L"Bytes Per Line"

#define WIA_IPA_NUMBER_OF_LINES                              4114 // 0x1012
#define WIA_IPA_NUMBER_OF_LINES_STR                          L"Number of Lines"

#define WIA_IPA_GAMMA_CURVES                                 4115 // 0x1013
#define WIA_IPA_GAMMA_CURVES_STR                             L"Gamma Curves"

#define WIA_IPA_ITEM_SIZE                                    4116 // 0x1014
#define WIA_IPA_ITEM_SIZE_STR                                L"Item Size"

#define WIA_IPA_COLOR_PROFILE                                4117 // 0x1015
#define WIA_IPA_COLOR_PROFILE_STR                            L"Color Profiles"

#define WIA_IPA_MIN_BUFFER_SIZE                              4118 // 0x1016
#define WIA_IPA_MIN_BUFFER_SIZE_STR                          L"Buffer Size"

#define WIA_IPA_BUFFER_SIZE                                  4118 // 0x1016
#define WIA_IPA_BUFFER_SIZE_STR                              L"Buffer Size"

#define WIA_IPA_REGION_TYPE                                  4119 // 0x1017
#define WIA_IPA_REGION_TYPE_STR                              L"Region Type"

#define WIA_IPA_ICM_PROFILE_NAME                             4120 // 0x1018
#define WIA_IPA_ICM_PROFILE_NAME_STR                         L"Color Profile Name"

#define WIA_IPA_APP_COLOR_MAPPING                            4121 // 0x1019
#define WIA_IPA_APP_COLOR_MAPPING_STR                        L"Application Applies Color Mapping"

#define WIA_IPA_PROP_STREAM_COMPAT_ID                        4122 // 0x101a
#define WIA_IPA_PROP_STREAM_COMPAT_ID_STR                    L"Stream Compatibility ID"

#define WIA_IPA_FILENAME_EXTENSION                           4123 // 0x101b
#define WIA_IPA_FILENAME_EXTENSION_STR                       L"Filename extension"

#define WIA_IPA_SUPPRESS_PROPERTY_PAGE                       4124 // 0x101c
#define WIA_IPA_SUPPRESS_PROPERTY_PAGE_STR                   L"Suppress a property page"

#define WIA_IPC_THUMBNAIL                                    5122 // 0x1402
#define WIA_IPC_THUMBNAIL_STR                                L"Thumbnail Data"

#define WIA_IPC_THUMB_WIDTH                                  5123 // 0x1403
#define WIA_IPC_THUMB_WIDTH_STR                              L"Thumbnail Width"

#define WIA_IPC_THUMB_HEIGHT                                 5124 // 0x1404
#define WIA_IPC_THUMB_HEIGHT_STR                             L"Thumbnail Height"

#define WIA_IPC_AUDIO_AVAILABLE                              5125 // 0x1405
#define WIA_IPC_AUDIO_AVAILABLE_STR                          L"Audio Available"

#define WIA_IPC_AUDIO_DATA_FORMAT                            5126 // 0x1406
#define WIA_IPC_AUDIO_DATA_FORMAT_STR                        L"Audio Format"

#define WIA_IPC_AUDIO_DATA                                   5127 // 0x1407
#define WIA_IPC_AUDIO_DATA_STR                               L"Audio Data"

#define WIA_IPC_NUM_PICT_PER_ROW                             5128 // 0x1408
#define WIA_IPC_NUM_PICT_PER_ROW_STR                         L"Pictures per Row"

#define WIA_IPC_SEQUENCE                                     5129 // 0x1409
#define WIA_IPC_SEQUENCE_STR                                 L"Sequence Number"

#define WIA_IPC_TIMEDELAY                                    5130 // 0x140a
#define WIA_IPC_TIMEDELAY_STR                                L"Time Delay"

#define WIA_IPS_CUR_INTENT                                   6146 // 0x1802
#define WIA_IPS_CUR_INTENT_STR                               L"Current Intent"

#define WIA_IPS_XRES                                         6147 // 0x1803
#define WIA_IPS_XRES_STR                                     L"Horizontal Resolution"

#define WIA_IPS_YRES                                         6148 // 0x1804
#define WIA_IPS_YRES_STR                                     L"Vertical Resolution"

#define WIA_IPS_XPOS                                         6149 // 0x1805
#define WIA_IPS_XPOS_STR                                     L"Horizontal Start Position"

#define WIA_IPS_YPOS                                         6150 // 0x1806
#define WIA_IPS_YPOS_STR                                     L"Vertical Start Position"

#define WIA_IPS_XEXTENT                                      6151 // 0x1807
#define WIA_IPS_XEXTENT_STR                                  L"Horizontal Extent"

#define WIA_IPS_YEXTENT                                      6152 // 0x1808
#define WIA_IPS_YEXTENT_STR                                  L"Vertical Extent"

#define WIA_IPS_PHOTOMETRIC_INTERP                           6153 // 0x1809
#define WIA_IPS_PHOTOMETRIC_INTERP_STR                       L"Photometric Interpretation"

#define WIA_IPS_BRIGHTNESS                                   6154 // 0x180a
#define WIA_IPS_BRIGHTNESS_STR                               L"Brightness"

#define WIA_IPS_CONTRAST                                     6155 // 0x180b
#define WIA_IPS_CONTRAST_STR                                 L"Contrast"

#define WIA_IPS_ORIENTATION                                  6156 // 0x180c
#define WIA_IPS_ORIENTATION_STR                              L"Orientation"

#define WIA_IPS_ROTATION                                     6157 // 0x180d
#define WIA_IPS_ROTATION_STR                                 L"Rotation"

#define WIA_IPS_MIRROR                                       6158 // 0x180e
#define WIA_IPS_MIRROR_STR                                   L"Mirror"

#define WIA_IPS_THRESHOLD                                    6159 // 0x180f
#define WIA_IPS_THRESHOLD_STR                                L"Threshold"

#define WIA_IPS_INVERT                                       6160 // 0x1810
#define WIA_IPS_INVERT_STR                                   L"Invert"

#define WIA_IPS_WARM_UP_TIME                                 6161 // 0x1811
#define WIA_IPS_WARM_UP_TIME_STR                             L"Lamp Warm up Time"

#if (_WIN32_WINNT >= 0x0600)

//
// New properties, property names and values specific to WIA 2.0 (introduced in Windows Vista):
//

#define WIA_DPS_USER_NAME                                    3112 // 0xc28
#define WIA_DPS_USER_NAME_STR                                L"User Name"

#define WIA_DPS_SERVICE_ID                                   3113 // 0xc29
#define WIA_DPS_SERVICE_ID_STR                               L"Service ID"

#define WIA_DPS_DEVICE_ID                                    3114 // 0xc2a
#define WIA_DPS_DEVICE_ID_STR                                L"Device ID"

#define WIA_DPS_GLOBAL_IDENTITY                              3115 // 0xc2b
#define WIA_DPS_GLOBAL_IDENTITY_STR                          L"Global Identity"

#define WIA_DPS_SCAN_AVAILABLE_ITEM                          3116 // 0xc2c
#define WIA_DPS_SCAN_AVAILABLE_ITEM_STR                      L"Scan Available Item"

#define WIA_IPS_DESKEW_X                                     6162 // 0x1812
#define WIA_IPS_DESKEW_X_STR                                 L"DeskewX"

#define WIA_IPS_DESKEW_Y                                     6163 // 0x1813
#define WIA_IPS_DESKEW_Y_STR                                 L"DeskewY"

#define WIA_IPS_SEGMENTATION                                 6164 // 0x1814
#define WIA_IPS_SEGMENTATION_STR                             L"Segmentation"

#define WIA_SEGMENTATION_FILTER_STR                          L"SegmentationFilter"
#define WIA_IMAGEPROC_FILTER_STR                             L"ImageProcessingFilter"

#define WIA_IPS_MAX_HORIZONTAL_SIZE                          6165 // 0x1815
#define WIA_IPS_MAX_HORIZONTAL_SIZE_STR                      L"Maximum Horizontal Scan Size"

#define WIA_IPS_MAX_VERTICAL_SIZE                            6166 // 0x1816
#define WIA_IPS_MAX_VERTICAL_SIZE_STR                        L"Maximum Vertical Scan Size"

#define WIA_IPS_MIN_HORIZONTAL_SIZE                          6167 // 0x1817
#define WIA_IPS_MIN_HORIZONTAL_SIZE_STR                      L"Minimum Horizontal Scan Size"

#define WIA_IPS_MIN_VERTICAL_SIZE                            6168 // 0x1818
#define WIA_IPS_MIN_VERTICAL_SIZE_STR                        L"Minimum Vertical Scan Size"

#define WIA_IPS_TRANSFER_CAPABILITIES                        6169 // 0x1819
#define WIA_IPS_TRANSFER_CAPABILITIES_STR                    L"Transfer Capabilities"

#define WIA_IPS_SHEET_FEEDER_REGISTRATION                    3078 // 0xc06
#define WIA_IPS_SHEET_FEEDER_REGISTRATION_STR                L"Sheet Feeder Registration"

#define WIA_IPS_DOCUMENT_HANDLING_SELECT                     3088 // 0xc10
#define WIA_IPS_DOCUMENT_HANDLING_SELECT_STR                 L"Document Handling Select"

#define WIA_IPS_OPTICAL_XRES                                 3090 // 0xc12
#define WIA_IPS_OPTICAL_XRES_STR                             L"Horizontal Optical Resolution"

#define WIA_IPS_OPTICAL_YRES                                 3091 // 0xc13
#define WIA_IPS_OPTICAL_YRES_STR                             L"Vertical Optical Resolution"

#define WIA_IPS_PAGES                                        3096 // 0xc18
#define WIA_IPS_PAGES_STR                                    L"Pages"

#define WIA_IPS_PAGE_SIZE                                    3097 // 0xc19
#define WIA_IPS_PAGE_SIZE_STR                                L"Page Size"

#define WIA_IPS_PAGE_WIDTH                                   3098 // 0xc1a
#define WIA_IPS_PAGE_WIDTH_STR                               L"Page Width"

#define WIA_IPS_PAGE_HEIGHT                                  3099 // 0xc1b
#define WIA_IPS_PAGE_HEIGHT_STR                              L"Page Height"

#define WIA_IPS_PREVIEW                                      3100 // 0xc1c
#define WIA_IPS_PREVIEW_STR                                  L"Preview"

#define WIA_IPS_SHOW_PREVIEW_CONTROL                         3103 // 0xc1f
#define WIA_IPS_SHOW_PREVIEW_CONTROL_STR                     L"Show preview control"

#define WIA_IPS_FILM_SCAN_MODE                               3104 // 0xc20
#define WIA_IPS_FILM_SCAN_MODE_STR                           L"Film Scan Mode"

#define WIA_IPS_LAMP                                         3105 // 0xc21
#define WIA_IPS_LAMP_STR                                     L"Lamp"

#define WIA_IPS_LAMP_AUTO_OFF                                3106 // 0xc22
#define WIA_IPS_LAMP_AUTO_OFF_STR                            L"Lamp Auto Off"

#define WIA_IPS_AUTO_DESKEW                                  3107 // 0xc23
#define WIA_IPS_AUTO_DESKEW_STR                              L"Automatic Deskew"

#define WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION                 3108 // 0xc24
#define WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION_STR             L"Supports Child Item Creation"

#define WIA_IPS_XSCALING                                     3109 // 0xc25
#define WIA_IPS_XSCALING_STR                                 L"Horizontal Scaling"

#define WIA_IPS_YSCALING                                     3110 // 0xc26
#define WIA_IPS_YSCALING_STR                                 L"Vertical Scaling"

#define WIA_IPS_PREVIEW_TYPE                                 3111 // 0xc27
#define WIA_IPS_PREVIEW_TYPE_STR                             L"Preview Type"

#define WIA_IPA_ITEM_CATEGORY                                4125 // 0x101d
#define WIA_IPA_ITEM_CATEGORY_STR                            L"Item Category"

#define WIA_IPA_UPLOAD_ITEM_SIZE                             4126 // 0x101e
#define WIA_IPA_UPLOAD_ITEM_SIZE_STR                         L"Upload Item Size"

#define WIA_IPA_ITEMS_STORED                                 4127 // 0x101f
#define WIA_IPA_ITEMS_STORED_STR                             L"Items Stored"

#define WIA_IPA_RAW_BITS_PER_CHANNEL                         4128 // 0x1020
#define WIA_IPA_RAW_BITS_PER_CHANNEL_STR                     L"Raw Bits Per Channel"

#define WIA_IPS_FILM_NODE_NAME                               4129 // 0x1021
#define WIA_IPS_FILM_NODE_NAME_STR                           L"Film Node Name"

#define WIA_IPS_PRINTER_ENDORSER                             4130 // 0x1022
#define WIA_IPS_PRINTER_ENDORSER_STR                         L"Printer/Endorser"

#define WIA_IPS_PRINTER_ENDORSER_ORDER                       4131 // 0x1023
#define WIA_IPS_PRINTER_ENDORSER_ORDER_STR                   L"Printer/Endorser Order"

#define WIA_IPS_PRINTER_ENDORSER_COUNTER                     4132 // 0x1024
#define WIA_IPS_PRINTER_ENDORSER_COUNTER_STR                 L"Printer/Endorser Counter"

#define WIA_IPS_PRINTER_ENDORSER_STEP                        4133 // 0x1025
#define WIA_IPS_PRINTER_ENDORSER_STEP_STR                    L"Printer/Endorser Step"

#define WIA_IPS_PRINTER_ENDORSER_XOFFSET                     4134 // 0x1026
#define WIA_IPS_PRINTER_ENDORSER_XOFFSET_STR                 L"Printer/Endorser Horizontal Offset"

#define WIA_IPS_PRINTER_ENDORSER_YOFFSET                     4135 // 0x1027
#define WIA_IPS_PRINTER_ENDORSER_YOFFSET_STR                 L"Printer/Endorser Vertical Offset"

#define WIA_IPS_PRINTER_ENDORSER_NUM_LINES                   4136 // 0x1028
#define WIA_IPS_PRINTER_ENDORSER_NUM_LINES_STR               L"Printer/Endorser Lines"

#define WIA_IPS_PRINTER_ENDORSER_STRING                      4137 // 0x1029
#define WIA_IPS_PRINTER_ENDORSER_STRING_STR                  L"Printer/Endorser String"

#define WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS            4138 // 0x102a
#define WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS_STR        L"Printer/Endorser Valid Characters"

#define WIA_IPS_PRINTER_ENDORSER_VALID_FORMAT_SPECIFIERS     4139 // 0x102b
#define WIA_IPS_PRINTER_ENDORSER_VALID_FORMAT_SPECIFIERS_STR L"Printer/Endorser Valid Format Specifiers"

#define WIA_IPS_PRINTER_ENDORSER_TEXT_UPLOAD                 4140 // 0x102c
#define WIA_IPS_PRINTER_ENDORSER_TEXT_UPLOAD_STR             L"Printer/Endorser Text Upload"

#define WIA_IPS_PRINTER_ENDORSER_TEXT_DOWNLOAD               4141 // 0x102d
#define WIA_IPS_PRINTER_ENDORSER_TEXT_DOWNLOAD_STR           L"Printer/Endorser Text Download"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS                    4142 // 0x102e
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_STR                L"Printer/Endorser Graphics"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_POSITION           4143 // 0x102f
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_POSITION_STR       L"Printer/Endorser Graphics Position"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_WIDTH          4144 // 0x1030
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_WIDTH_STR      L"Printer/Endorser Graphics Minimum Width"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_WIDTH          4145 // 0x1031
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_WIDTH_STR      L"Printer/Endorser Graphics Maximum Width"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_HEIGHT         4146 // 0x1032
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_HEIGHT_STR     L"Printer/Endorser Graphics Minimum Height"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_HEIGHT         4147 // 0x1033
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_HEIGHT_STR     L"Printer/Endorser Graphics Maximum Height"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_UPLOAD             4148 // 0x1034
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_UPLOAD_STR         L"Printer/Endorser Graphics Upload"

#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_DOWNLOAD           4149 // 0x1035
#define WIA_IPS_PRINTER_ENDORSER_GRAPHICS_DOWNLOAD_STR       L"Printer/Endorser Graphics Download"

#define WIA_IPS_BARCODE_READER                               4150 // 0x1036
#define WIA_IPS_BARCODE_READER_STR                           L"Barcode Reader"

#define WIA_IPS_MAXIMUM_BARCODES_PER_PAGE                    4151 // 0x1037
#define WIA_IPS_MAXIMUM_BARCODES_PER_PAGE_STR                L"Maximum Barcodes Per Page"

#define WIA_IPS_BARCODE_SEARCH_DIRECTION                     4152 // 0x1038
#define WIA_IPS_BARCODE_SEARCH_DIRECTION_STR                 L"Barcode Search Direction"

#define WIA_IPS_MAXIMUM_BARCODE_SEARCH_RETRIES               4153 // 0x1039
#define WIA_IPS_MAXIMUM_BARCODE_SEARCH_RETRIES_STR           L"Barcode Search Retries"

#define WIA_IPS_BARCODE_SEARCH_TIMEOUT                       4154 // 0x103a
#define WIA_IPS_BARCODE_SEARCH_TIMEOUT_STR                   L"Barcode Search Timeout"

#define WIA_IPS_SUPPORTED_BARCODE_TYPES                      4155 // 0x103b
#define WIA_IPS_SUPPORTED_BARCODE_TYPES_STR                  L"Supported Barcode Types"

#define WIA_IPS_ENABLED_BARCODE_TYPES                        4156 // 0x103c
#define WIA_IPS_ENABLED_BARCODE_TYPES_STR                    L"Enabled Barcode Types"

#define WIA_IPS_PATCH_CODE_READER                            4157 // 0x103d
#define WIA_IPS_PATCH_CODE_READER_STR                        L"Patch Code Reader"

#define WIA_IPS_SUPPORTED_PATCH_CODE_TYPES                   4162 // 0x1042
#define WIA_IPS_SUPPORTED_PATCH_CODE_TYPES_STR               L"Supported Patch Code Types"

#define WIA_IPS_ENABLED_PATCH_CODE_TYPES                     4163 // 0x1043
#define WIA_IPS_ENABLED_PATCH_CODE_TYPES_STR                 L"Enabled Path Code Types"

#define WIA_IPS_MICR_READER                                  4164 // 0x1044
#define WIA_IPS_MICR_READER_STR                              L"MICR Reader"

#define WIA_IPS_JOB_SEPARATORS                               4165 // 0x1045
#define WIA_IPS_JOB_SEPARATORS_STR                           L"Job Separators"

#define WIA_IPS_LONG_DOCUMENT                                4166 // 0x1046
#define WIA_IPS_LONG_DOCUMENT_STR                            L"Long Document"

#define WIA_IPS_BLANK_PAGES                                  4167 // 0x1047
#define WIA_IPS_BLANK_PAGES_STR                              L"Blank Pages"

#define WIA_IPS_MULTI_FEED                                   4168 // 0x1048
#define WIA_IPS_MULTI_FEED_STR                               L"Multi-Feed"

#define WIA_IPS_MULTI_FEED_SENSITIVITY                       4169 // 0x1049
#define WIA_IPS_MULTI_FEED_SENSITIVITY_STR                   L"Multi-Feed Sensitivity"

#define WIA_IPS_AUTO_CROP                                    4170 // 0x104a
#define WIA_IPS_AUTO_CROP_STR                                L"Auto-Crop"

#define WIA_IPS_OVER_SCAN                                    4171 // 0x104b
#define WIA_IPS_OVER_SCAN_STR                                L"Overscan"

#define WIA_IPS_OVER_SCAN_LEFT                               4172 // 0x104c
#define WIA_IPS_OVER_SCAN_LEFT_STR                           L"Overscan Left"

#define WIA_IPS_OVER_SCAN_RIGHT                              4173 // 0x104d
#define WIA_IPS_OVER_SCAN_RIGHT_STR                          L"Overscan Right"

#define WIA_IPS_OVER_SCAN_TOP                                4174 // 0x104e
#define WIA_IPS_OVER_SCAN_TOP_STR                            L"Overscan Top"

#define WIA_IPS_OVER_SCAN_BOTTOM                             4175 // 0x104f
#define WIA_IPS_OVER_SCAN_BOTTOM_STR                         L"Overscan Bottom"

#define WIA_IPS_COLOR_DROP                                   4176 // 0x1050
#define WIA_IPS_COLOR_DROP_STR                               L"Color Drop"

#define WIA_IPS_COLOR_DROP_RED                               4177 // 0x1051
#define WIA_IPS_COLOR_DROP_RED_STR                           L"Color Drop Red"

#define WIA_IPS_COLOR_DROP_GREEN                             4178 // 0x1052
#define WIA_IPS_COLOR_DROP_GREEN_STR                         L"Color Drop Green"

#define WIA_IPS_COLOR_DROP_BLUE                              4179 // 0x1053
#define WIA_IPS_COLOR_DROP_BLUE_STR                          L"Color Drop Blue"

#define WIA_IPS_SCAN_AHEAD                                   4180 // 0x1054
#define WIA_IPS_SCAN_AHEAD_STR                               L"Scan Ahead"

#define WIA_IPS_SCAN_AHEAD_CAPACITY                          4181 // 0x1055
#define WIA_IPS_SCAN_AHEAD_CAPACITY_STR                      L"Scan Ahead Capacity"

#define WIA_IPS_FEEDER_CONTROL                               4182 // 0x1056
#define WIA_IPS_FEEDER_CONTROL_STR                           L"Feeder Control"

#define WIA_IPS_PRINTER_ENDORSER_PADDING                     4183 // 0x1057
#define WIA_IPS_PRINTER_ENDORSER_PADDING_STR                 L"Printer/Endorser Padding"

#define WIA_IPS_PRINTER_ENDORSER_FONT_TYPE                   4184 // 0x1058
#define WIA_IPS_PRINTER_ENDORSER_FONT_TYPE_STR               L"Printer/Endorser Font Type"

#define WIA_IPS_ALARM                                        4185 // 0x1059
#define WIA_IPS_ALARM_STR                                    L"Alarm"

#define WIA_IPS_PRINTER_ENDORSER_INK                         4186 // 0x105A
#define WIA_IPS_PRINTER_ENDORSER_INK_STR                     L"Printer/Endorser Ink"

#define WIA_IPS_PRINTER_ENDORSER_CHARACTER_ROTATION          4187 // 0x105B
#define WIA_IPS_PRINTER_ENDORSER_CHARACTER_ROTATION_STR      L"Printer/Endorser Character Rotation"

#define WIA_IPS_PRINTER_ENDORSER_MAX_CHARACTERS              4188 // 0x105C
#define WIA_IPS_PRINTER_ENDORSER_MAX_CHARACTERS_STR          L"Printer/Endorser Maximum Characters"

#define WIA_IPS_PRINTER_ENDORSER_MAX_GRAPHICS                4189 // 0x105D
#define WIA_IPS_PRINTER_ENDORSER_MAX_GRAPHICS_STR            L"Printer/Endorser Maximum Graphics"

#define WIA_IPS_PRINTER_ENDORSER_COUNTER_DIGITS              4190 // 0x105E
#define WIA_IPS_PRINTER_ENDORSER_COUNTER_DIGITS_STR          L"Printer/Endorser Counter Digits"

#define WIA_IPS_COLOR_DROP_MULTI                             4191 // 0x105F
#define WIA_IPS_COLOR_DROP_MULTI_STR                         L"Color Drop Multiple"

#define WIA_IPS_BLANK_PAGES_SENSITIVITY                      4192 // 0x1060
#define WIA_IPS_BLANK_PAGES_SENSITIVITY_STR                  L"Blank Pages Sensitivity"

#define WIA_IPS_MULTI_FEED_DETECT_METHOD                     4193 // 0x1061
#define WIA_IPS_MULTI_FEED_DETECT_METHOD_STR                 L"Multi-Feed Detection Method"

..........

#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef __cplusplus
};
#endif

#include <poppack.h>

#endif // _WIADEF_

#endif //#if (_WIN32_WINNT >= 0x0501)
