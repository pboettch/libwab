/*
*  This file was generated automatically by makeids.pl
*
*  if you want to modify anything in this file then modify known_ids and then run makeids.pl
*    ...and send me a copy :)
*
*/

#ifndef PSTWABIDS_H
#define PSTWABIDS_H

char *id_get_str( int id );
char *ldid_get_str( int id );

#define MT_SINT16   0x0002 //Signed 16bit value
#define MT_SINT32   0x0003 //Signed 32bit value
#define MT_BOOL     0x000B //Boolean (non-zero = true)
#define MT_EMBEDDED 0x000D //Embedded Object
#define MT_STRING   0x001E //Null terminated String
#define MT_UNICODE  0x001F //Unicode string
#define MT_SYSTIME  0x0040 //Systime - Filetime structure
#define MT_OLE_GRID 0x0048 //OLE Guid
#define MT_BINARY   0x0102 //Binary data

#define MT_SINT32_ARRAY  0x1003 // Array of 32bit values
#define MT_STRING_ARRAY  0x101E // Array of Strings
#define MT_UNICODE_ARRAY 0x101F // Array of Unicode
#define MT_BINARY_ARRAY  0x1102 // Array of Binary data


#define PR_ALTERNATE_RECIPIENT_ALLOWED_STR                           0x0002
#define PR_IMPORTANCE_STR                                            0x0017
#define PR_MESSAGE_CLASS_STR                                         0x001A
#define PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED_STR                  0x0023
#define PR_PRIORITY_STR                                              0x0026
#define PR_READ_RECEIPT_REQUESTED_STR                                0x0029
#define PR_ORIGINAL_SENSITIVITY_STR                                  0x002E
#define PR_SENSITIVITY_STR                                           0x0036
#define PR_SUBJECT_STR                                               0x0037
#define PR_CLIENT_SUBMIT_TIME_STR                                    0x0039
#define PR_SENT_REPRESENTING_SEARCH_KEY_STR                          0x003B
#define PR_RECEIVED_BY_ENTRYID_STR                                   0x003F
#define PR_RECEIVED_BY_NAME_STR                                      0x0040
#define PR_SENT_REPRESENTING_ENTRYID_STR                             0x0041
#define PR_SENT_REPRESENTING_NAME_STR                                0x0042
#define PR_RCVD_REPRESENTING_ENTRYID_STR                             0x0043
#define PR_RCVD_REPRESENTING_NAME_STR                                0x0044
#define PR_REPLY_RECIPIENT_ENTRIES_STR                               0x004F
#define PR_REPLY_RECIPIENT_NAMES_STR                                 0x0050
#define PR_RECEIVED_BY_SEARCH_KEY_STR                                0x0051
#define PR_RCVD_REPRESENTING_SEARCH_KEY_STR                          0x0052
#define PR_MESSAGE_TO_ME_STR                                         0x0057
#define PR_MESSAGE_CC_ME_STR                                         0x0058
#define PR_MESSAGE_RECIP_ME_STR                                      0x0059
#define PR_SENT_REPRESENTING_ADDRTYPE_STR                            0x0064
#define PR_SENT_REPRESENTING_EMAIL_ADDRESS_STR                       0x0065
#define PR_CONVERSATION_TOPIC_STR                                    0x0070
#define PR_CONVERSATION_INDEX_STR                                    0x0071
#define PR_RECEIVED_BY_ADDRTYPE_STR                                  0x0075
#define PR_RECEIVED_BY_EMAIL_ADDRESS_STR                             0x0076
#define PR_RCVD_REPRESENTING_ADDRTYPE_STR                            0x0077
#define PR_RCVD_REPRESENTING_EMAIL_ADDRESS_STR                       0x0078
#define PR_TRANSPORT_MESSAGE_HEADERS_STR                             0x007D
#define PR_SENDER_ENTRYID_STR                                        0x0C19
#define PR_SENDER_NAME_STR                                           0x0C1A
#define PR_SENDER_SEARCH_KEY_STR                                     0x0C1D
#define PR_SENDER_ADDRTYPE_STR                                       0x0C1E
#define PR_SENDER_EMAIL_ADDRESS_STR                                  0x0C1F
#define PR_DELETE_AFTER_SUBMIT_STR                                   0x0E01
#define PR_DISPLAY_CC_STR                                            0x0E03
#define PR_DISPLAY_TO_STR                                            0x0E04
#define PR_MESSAGE_DELIVERY_TIME_STR                                 0x0E06
#define PR_MESSAGE_FLAGS_STR                                         0x0E07
#define PR_MESSAGE_SIZE_STR                                          0x0E08
#define PR_SENTMAIL_ENTRYID_STR                                      0x0E0A
#define PR_RTF_IN_SYNC_STR                                           0x0E1F
#define PR_ATTACH_SIZE_STR                                           0x0E20
#define PR_RECORD_KEY_STR                                            0x0FF9
#define PR_MAB_MYSTERY_ALWAYS_6                                      0x0ffe
#define PR_MAB_ENTRY_ID                                              0x0fff
#define PR_BODY_STR                                                  0x1000
#define PR_RTF_SYNC_BODY_CRC_STR                                     0x1006
#define PR_RTF_SYNC_BODY_COUNT_STR                                   0x1007
#define PR_RTF_SYNC_BODY_TAG_STR                                     0x1008
#define PR_RTF_COMPRESSED_STR                                        0x1009
#define PR_RTF_SYNC_PREFIX_COUNT_STR                                 0x1010
#define PR_RTF_SYNC_TRAILING_COUNT_STR                               0x1011
#define PR_HTML_BODY_STR                                             0x1013
#define PR_MESSAGE_ID_STR                                            0x1035
#define PR_IN_REPLY_TO_STR                                           0x1042
#define PR_RETURN_PATH_STR                                           0x1046
#define PR_DISPLAY_NAME                                              0x3001
#define PR_MAB_SEND_METHOD                                           0x3002
#define PR_MAB_ADDRESS_STR                                           0x3003
#define PR_COMMENT_STR                                               0x3004
#define PR_CREATION_TIME_STR                                         0x3007
#define PR_LAST_MODIFICATION_TIME_STR                                0x3008
#define PR_SEARCH_KEY_STR                                            0x300B
#define PR_VALID_FOLDER_MASK_STR                                     0x35DF
#define PR_IPM_SUBTREE_ENTRYID_STR                                   0x35E0
#define PR_IPM_WASTEBASKET_ENTRYID_STR                               0x35E3
#define PR_FINDER_ENTRYID_STR                                        0x35E7
#define PR_CONTENT_COUNT_STR                                         0x3602
#define PR_CONTENT_UNREAD_STR                                        0x3603
#define PR_SUBFOLDERS_STR                                            0x360A
#define PR_CONTAINER_CLASS_STR                                       0x3613
#define PR_ASSOC_CONTENT_COUNT_STR                                   0x3617
#define PR_ATTACH_DATA_OBJ_STR                                       0x3701
#define PR_ATTACH_FILENAME_STR                                       0x3704
#define PR_ATTACH_METHOD_STR                                         0x3705
#define PR_ATTACH_LONG_FILENAME_STR                                  0x3707
#define PR_RENDERING_POSITION_STR                                    0x370B
#define PR_ATTACH_MIME_TAG_STR                                       0x370E
#define PR_ATTACH_MIME_SEQUENCE_STR                                  0x3710
#define PR_GIVEN_NAME_STR                                            0x3a06
#define PR_MAB_BUSINESS_PHONE                                        0x3a08
#define PR_MAB_PHONE                                                 0x3a09
#define PR_INITIALS_STR                                              0x3a0a
#define PR_SURNAME_STR                                               0x3a11
#define PR_MAB_COMPANY                                               0x3a16
#define PR_MAB_JOBTITLE                                              0x3a17
#define PR_MAB_DEPARTMENT                                            0x3a18
#define PR_MAB_OFFICE                                                0x3a19
#define PR_MAB_MOBILE                                                0x3a1c
#define PR_MAB_PAGER                                                 0x3a21
#define PR_MAB_BUSINESS_FAX                                          0x3a24
#define PR_MAB_FAX                                                   0x3a25
#define PR_MAB_BUSINESS_COUNTRY                                      0x3a26
#define PR_MAB_BUSINESS_CITY                                         0x3a27
#define PR_MAB_BUSINESS_PROVINCE                                     0x3a28
#define PR_MAB_BUSINESS_ADDRESS                                      0x3a29
#define PR_MAB_BUSINESS_POSTAL_CODE                                  0x3a2a
#define PR_MAB_BUSINESS_MIDDLE                                       0x3a44
#define PR_MAB_BUSINESS_TITLE                                        0x3a45
#define PR_MAB_NICK                                                  0x3a4f
#define PR_MAB_BUSINESS_URL1                                         0x3a50
#define PR_MAB_BUSINESS_URL2                                         0x3a51
#define PR_MAB_ALTERNATE_EMAIL_TYPES                                 0x3a54
#define PR_MAB_MYSTERY_ALWAYS_0_FIRST                                0x3a55
#define PR_MAB_ALTERNATE_EMAILS                                      0x3a56
#define PR_MAB_CITY                                                  0x3a59
#define PR_MAB_COUNTRY                                               0x3a5a
#define PR_MAB_POSTAL_CODE                                           0x3a5b
#define PR_MAB_PROVINCE                                              0x3a5c
#define PR_MAB_STREET_ADDRESS                                        0x3a5d
#define PR_MAB_MYSTERY_ALWAYS_1048576                                0x3a71
#define PR_MAB_MYSTERY_PROFILE_ID1                                   0x8004
#define PR_MAB_MEMBER                                                0x8009
#define PR_MAB_IP_PHONE                                              0x800a
#define PR_MAB_MYSTERY_PROFILE_ID2                                   0x800d
#define PR_MAB_EMAIL_ADDRESS_WTF                                     0x8012
#define PR_Schedule_Folder_EntryID_STR                               0x661E
#define PR_ID2_STR                                                   0x67F2
#define PR_EXTRA_PROPERTY_IDENTIFIER_STR                             0x67FF
#define PR_MAB_MYSTERY_ALWAYS_0_NEXT                                 0x800c
#define PR_ADDRESS_1_STR                                             0x8019
#define PR_ACCESS_METHOD_STR                                         0x80cf
#define PR_ADDRESS_1_DESCRIPTION_STR                                 0x80d1
#endif
