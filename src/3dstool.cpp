#include "3dstool.h"
#include "3dscrypt.h"
#include "backwardlz77.h"
#include "banner.h"
#include "exefs.h"
#include "huffman.h"
#include "lz77.h"
#include "ncch.h"
#include "ncsd.h"
#include "patch.h"
#include "romfs.h"
#include "runlength.h"
#include "yaz0.h"

C3dsTool::SOption C3dsTool::s_Option[] =
{
	{ nullptr, 0, "action:" },
	{ "extract", 'x', "extract the target file" },
	{ "create", 'c', "create the target file" },
	{ "encrypt", 'e', "encrypt the target file" },
	{ "uncompress", 'u', "uncompress the target file" },
	{ "compress", 'z', "compress the target file" },
	{ "trim", 'r', "trim the cci file" },
	{ "pad", 'p', "pad the cci file" },
	{ "diff", 0, "create the patch file from the old file and the new file" },
	{ "patch", 0, "apply the patch file to the target file" },
	{ "sample", 0, "show the samples" },
	{ "help", 'h', "show this help" },
	{ nullptr, 0, "\ncommon:" },
	{ "type", 't', "[[card|cci|3ds]|[exec|cxi]|[data|cfa]|exefs|romfs|banner]\n\t\tthe type of the file, optional" },
	{ "file", 'f', "the target file, required" },
	{ "verbose", 'v', "show the info" },
	{ nullptr, 0, " extract/create:" },
	{ nullptr, 0, "  cci/cxi/cfa/exefs:" },
	{ "header", 0, "the header file of the target file" },
	{ nullptr, 0, " encrypt:" },
	{ "key0", 0, "short for --key 00000000000000000000000000000000" },
	{ "key", 0, "the hex string of the key used by the AES-CTR encryption" },
	{ "counter", 0, "the hex string of the counter used by the AES-CTR encryption" },
	{ "xor", 0, "the xor data file used by the xor encryption" },
	{ nullptr, 0, " compress:" },
	{ "compress-align", 0, "[1|4|8|16|32]\n\t\tthe alignment of the compressed filesize, optional" },
	{ nullptr, 0, "  uncompress:" },
	{ "compress-type", 0, "[blz|lz(ex)|h4|h8|rl|yaz0]\n\t\tthe type of the compress" },
	{ "compress-out", 0, "the output file of uncompressed or compressed" },
	{ nullptr, 0, "  yaz0:" },
	{ "yaz0-align", 0, "[0|128]\n\t\tthe alignment property of the yaz0 compressed file, optional" },
	{ nullptr, 0, " diff:" },
	{ "old", 0, "the old file" },
	{ "new", 0, "the new file" },
	{ nullptr, 0, "  patch:" },
	{ "patch-file", 0, "the patch file" },
	{ nullptr, 0, "\ncci:" },
	{ nullptr, 0, " create:" },
	{ "not-pad", 0, "do not add the pad data" },
	{ nullptr, 0, "  extract:" },
	{ "partition0", '0', "the cxi file of the cci file at partition 0" },
	{ "partition1", '1', "the cfa file of the cci file at partition 1" },
	{ "partition2", '2', "the cfa file of the cci file at partition 2" },
	{ "partition3", '3', "the cfa file of the cci file at partition 3" },
	{ "partition4", '4', "the cfa file of the cci file at partition 4" },
	{ "partition5", '5', "the cfa file of the cci file at partition 5" },
	{ "partition6", '6', "the cfa file of the cci file at partition 6" },
	{ "partition7", '7', "the cfa file of the cci file at partition 7" },
	{ nullptr, 0, " trim:" },
	{ "trim-after-partition", 0, "[0~7], the index of the last reserve partition, optional" },
	{ nullptr, 0, "\ncxi:" },
	{ nullptr, 0, " create:" },
	{ "not-update-exh-hash", 0, nullptr },
	{ "not-update-extendedheader-hash", 0, "do not update the extendedheader hash" },
	{ nullptr, 0, "  extract:" },
	{ "exh", 0, nullptr },
	{ "extendedheader", 0, "the extendedheader file of the cxi file" },
	{ "logo", 0, nullptr },
	{ "logoregion", 0, "the logoregion file of the cxi file" },
	{ "plain", 0, nullptr },
	{ "plainregion", 0, "the plainregion file of the cxi file" },
	{ nullptr, 0, "   encrypt:" },
	{ "exh-xor", 0, nullptr },
	{ "extendedheader-xor", 0, "the xor data file used by encrypt the extendedheader of the cxi file" },
	{ "exefs-top-xor", 0, "the xor data file used by encrypt the top section of the exefs of the cxi file" },
	{ "exefs-top-auto-key", 0, "use the known key to encrypt the top section of the exefs of the cxi file" },
	{ nullptr, 0, " cfa:" },
	{ nullptr, 0, "  create:" },
	{ "not-update-exefs-hash", 0, "do not update the exefs super block hash" },
	{ "not-update-romfs-hash", 0, "do not update the romfs super block hash" },
	{ nullptr, 0, "   extract:" },
	{ "exefs", 0, "the exefs file of the cxi/cfa file" },
	{ "romfs", 0, "the romfs file of the cxi/cfa file" },
	{ nullptr, 0, "    encrypt:" },
	{ "exefs-xor", 0, "the xor data file used by encrypt the exefs of the cxi/cfa file" },
	{ "romfs-xor", 0, "the xor data file used by encrypt the romfs of the cxi/cfa file" },
	{ "romfs-auto-key", 0, "use the known key to encrypt the romfs of the cxi/cfa file" },
	{ nullptr, 0, "\nexefs:" },
	{ nullptr, 0, " extract/create:" },
	{ "exefs-dir", 0, "the exefs dir for the exefs file" },
	{ nullptr, 0, "\nromfs:" },
	{ nullptr, 0, " extract/create:" },
	{ "romfs-dir", 0, "the romfs dir for the romfs file" },
	{ nullptr, 0, "\nbanner:" },
	{ nullptr, 0, " extract/create:" },
	{ "banner-dir", 0, "the banner dir for the banner file" },
	{ nullptr, 0, nullptr }
};

C3dsTool::C3dsTool()
	: m_eAction(kActionNone)
	, m_eFileType(kFileTypeUnknown)
	, m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_pHeaderFileName(nullptr)
	, m_nEncryptMode(CNcch::kEncryptModeNone)
	, m_nCompressAlign(1)
	, m_eCompressType(kCompressTypeNone)
	, m_nYaz0Align(0)
	, m_bNotPad(false)
	, m_nLastPartitionIndex(7)
	, m_bNotUpdateExtendedHeaderHash(false)
	, m_bNotUpdateExeFsHash(false)
	, m_bNotUpdateRomFsHash(false)
	, m_bExeFsTopAutoKey(false)
	, m_bRomFsAutoKey(false)
	, m_bCounterValid(false)
	, m_bUncompress(false)
	, m_bCompress(false)
{
}

C3dsTool::~C3dsTool()
{
}

int C3dsTool::ParseOptions(int a_nArgc, char* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(strlen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != '-')
		{
			printf("ERROR: illegal option\n\n");
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != '-')
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnUnknownArgument:
					printf("ERROR: unknown argument \"%s\"\n\n", m_sMessage.c_str());
					return 1;
				case kParseOptionReturnOptionConflict:
					printf("ERROR: option conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == '-')
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				printf("ERROR: illegal option\n\n");
				return 1;
			case kParseOptionReturnNoArgument:
				printf("ERROR: no argument\n\n");
				return 1;
			case kParseOptionReturnUnknownArgument:
				printf("ERROR: unknown argument \"%s\"\n\n", m_sMessage.c_str());
				return 1;
			case kParseOptionReturnOptionConflict:
				printf("ERROR: option conflict\n\n");
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int C3dsTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		printf("ERROR: nothing to do\n\n");
		return 1;
	}
	if (m_eAction != kActionDiff && m_eAction != kActionSample && m_eAction != kActionHelp && m_pFileName == nullptr)
	{
		printf("ERROR: no --file option\n\n");
		return 1;
	}
	if (m_eAction == kActionExtract)
	{
		if (!checkFileType())
		{
			printf("ERROR: %s\n\n", m_sMessage.c_str());
			return 1;
		}
		switch (m_eFileType)
		{
		case kFileTypeCci:
			if (m_pHeaderFileName == nullptr)
			{
				bool bEmpty = true;
				for (int i = 0; i < 8; i++)
				{
					if (!m_mNcchFileName[i].empty())
					{
						bEmpty = false;
						break;
					}
				}
				if (bEmpty)
				{
					printf("ERROR: nothing to be extract\n\n");
					return 1;
				}
			}
			break;
		case kFileTypeCxi:
			if (m_pHeaderFileName == nullptr && m_sExtendedHeaderFileName.empty() && m_sLogoRegionFileName.empty() && m_sPlainRegionFileName.empty() && m_sExeFsFileName.empty() && m_sRomFsFileName.empty())
			{
				printf("ERROR: nothing to be extract\n\n");
				return 1;
			}
			break;
		case kFileTypeCfa:
			if (m_pHeaderFileName == nullptr && m_sRomFsFileName.empty())
			{
				printf("ERROR: nothing to be extract\n\n");
				return 1;
			}
			break;
		case kFileTypeExeFs:
			if (m_pHeaderFileName == nullptr && m_sExeFsDirName.empty())
			{
				printf("ERROR: nothing to be extract\n\n");
				return 1;
			}
			break;
		case kFileTypeRomFs:
			if (m_sRomFsDirName.empty())
			{
				printf("ERROR: no --romfs-dir option\n\n");
				return 1;
			}
			break;
		case kFileTypeBanner:
			if (m_sBannerDirName.empty())
			{
				printf("ERROR: no --banner-dir option\n\n");
				return 1;
			}
			break;
		default:
			break;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (m_eFileType == kFileTypeUnknown)
		{
			printf("ERROR: no --type option\n\n");
			return 1;
		}
		else
		{
			if (m_eFileType == kFileTypeCci || m_eFileType == kFileTypeCxi || m_eFileType == kFileTypeCfa || m_eFileType == kFileTypeExeFs)
			{
				if (m_pHeaderFileName == nullptr)
				{
					printf("ERROR: no --header option\n\n");
					return 1;
				}
			}
			if (m_eFileType == kFileTypeCci)
			{
				if (m_mNcchFileName[0].empty())
				{
					printf("ERROR: no --partition0 option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeCxi)
			{
				if (m_sExtendedHeaderFileName.empty() || m_sExeFsFileName.empty())
				{
					printf("ERROR: no --extendedheader or --exefs option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeCfa)
			{
				if (m_sRomFsFileName.empty())
				{
					printf("ERROR: no --romfs option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeExeFs)
			{
				if (m_sExeFsDirName.empty())
				{
					printf("ERROR: no --exefs-dir option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeRomFs)
			{
				if (m_sRomFsDirName.empty())
				{
					printf("ERROR: no --romfs-dir option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeBanner)
			{
				if (m_sBannerDirName.empty())
				{
					printf("ERROR: no --banner-dir option\n\n");
					return 1;
				}
			}
		}
	}
	if (m_eAction == kActionEncrypt)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			printf("ERROR: no key or xor data file\n\n");
			return 1;
		}
		else if (m_nEncryptMode == CNcch::kEncryptModeXor)
		{
			if (!m_sExtendedHeaderXorFileName.empty() || !m_sExeFsXorFileName.empty() || !m_sRomFsXorFileName.empty())
			{
				if (!m_sXorFileName.empty())
				{
					printf("ERROR: --xor can not with --extendedheader-xor or --exefs-xor or --romfs-xor\n\n");
					return 1;
				}
				if (CNcch::IsCxiFile(m_pFileName))
				{
					if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCxi && m_bVerbose)
					{
						printf("INFO: ignore --type option\n");
					}
				}
				else if (CNcch::IsCfaFile(m_pFileName))
				{
					if (m_sRomFsXorFileName.empty())
					{
						printf("ERROR: no --romfs-xor option\n\n");
						return 1;
					}
					if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCfa && m_bVerbose)
					{
						printf("INFO: ignore --type option\n");
					}
				}
				else
				{
					printf("ERROR: %s is not a ncch file\n\n", m_pFileName);
					return 1;
				}
			}
		}
	}
	if (m_eAction == kActionUncompress || m_eAction == kActionCompress)
	{
		if (m_eCompressType == kCompressTypeNone)
		{
			printf("ERROR: no --compress-type option\n\n");
			return 1;
		}
		if (m_sCompressOutFileName.empty())
		{
			m_sCompressOutFileName = m_pFileName;
		}
	}
	if (m_eAction == kActionTrim || m_eAction == kActionPad)
	{
		if (!CNcsd::IsNcsdFile(m_pFileName))
		{
			printf("ERROR: %s is not a ncsd file\n\n", m_pFileName);
			return 1;
		}
		else if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCci && m_bVerbose)
		{
			printf("INFO: ignore --type option\n");
		}
	}
	if (m_eAction == kActionDiff)
	{
		if (m_sOldFileName.empty())
		{
			printf("ERROR: no --old option\n\n");
			return 1;
		}
		if (m_sNewFileName.empty())
		{
			printf("ERROR: no --new option\n\n");
			return 1;
		}
		if (m_sPatchFileName.empty())
		{
			printf("ERROR: no --patch-file option\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionPatch)
	{
		if (m_sPatchFileName.empty())
		{
			printf("ERROR: no --patch-file option\n\n");
			return 1;
		}
	}
	return 0;
}

int C3dsTool::Help()
{
	printf("3dstool %s by dnasdw\n\n", _3DSTOOL_VERSION);
	printf("usage: 3dstool [option...] [option]...\n\n");
	printf("option:\n");
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			printf("  ");
			if (pOption->Key != 0)
			{
				printf("-%c,", pOption->Key);
			}
			else
			{
				printf("   ");
			}
			printf(" --%-8s", pOption->Name);
			if (strlen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				printf("\n%16s", "");
			}
		}
		if (pOption->Doc != nullptr)
		{
			printf("%s", pOption->Doc);
		}
		printf("\n");
		pOption++;
	}
	return 0;
}

int C3dsTool::Action()
{
	if (m_eAction == kActionExtract)
	{
		if (!extractFile())
		{
			printf("ERROR: extract file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (!createFile())
		{
			printf("ERROR: create file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionEncrypt)
	{
		if (!encryptFile())
		{
			printf("ERROR: encrypt file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionUncompress)
	{
		if (!uncompressFile())
		{
			printf("ERROR: uncompress file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionCompress)
	{
		if (!compressFile())
		{
			printf("ERROR: compress file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionTrim)
	{
		if (!trimFile())
		{
			printf("ERROR: trim file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionPad)
	{
		if (!padFile())
		{
			printf("ERROR: pad file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionDiff)
	{
		if (!diffFile())
		{
			printf("ERROR: create patch file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionPatch)
	{
		if (!patchFile())
		{
			printf("ERROR: apply patch file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionSample)
	{
		return sample();
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

C3dsTool::EParseOptionReturn C3dsTool::parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[])
{
	if (strcmp(a_pName, "extract") == 0)
	{
		if (m_eAction == kActionNone || m_eAction == kActionUncompress)
		{
			m_eAction = kActionExtract;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "create") == 0)
	{
		if (m_eAction == kActionNone || m_eAction == kActionCompress)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "encrypt") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionEncrypt;
		}
		else if (m_eAction != kActionEncrypt && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "uncompress") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionUncompress;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionUncompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bUncompress = true;
	}
	else if (strcmp(a_pName, "compress") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCompress;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionCompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bCompress = true;
	}
	else if (strcmp(a_pName, "trim") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionTrim;
		}
		else if (m_eAction != kActionTrim && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "pad") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionPad;
		}
		else if (m_eAction != kActionPad && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "diff") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionDiff;
		}
		else if (m_eAction != kActionDiff && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "patch") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionPatch;
		}
		else if (m_eAction != kActionPatch && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "sample") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionSample;
		}
		else if (m_eAction != kActionSample && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "help") == 0)
	{
		m_eAction = kActionHelp;
	}
	else if (strcmp(a_pName, "type") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		string sType = a_pArgv[++a_nIndex];
		if (sType == "card" || sType == "cci" || sType == "3ds")
		{
			m_eFileType = kFileTypeCci;
		}
		else if (sType == "exec" || sType == "cxi")
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (sType == "data" || sType == "cfa")
		{
			m_eFileType = kFileTypeCfa;
		}
		else if (sType == "exefs")
		{
			m_eFileType = kFileTypeExeFs;
		}
		else if (sType == "romfs")
		{
			m_eFileType = kFileTypeRomFs;
		}
		else if (sType == "banner")
		{
			m_eFileType = kFileTypeBanner;
		}
		else
		{
			m_sMessage = sType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "verbose") == 0)
	{
		m_bVerbose = true;
	}
	else if (strcmp(a_pName, "header") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "key0") == 0)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_Key = 0;
	}
	else if (strcmp(a_pName, "key") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		string sKey = a_pArgv[++a_nIndex];
		if (sKey.size() != 32 || sKey.find_first_not_of("0123456789ABCDEFabcdef") != string::npos)
		{
			return kParseOptionReturnUnknownArgument;
		}
		m_Key = sKey.c_str();
	}
	else if (strcmp(a_pName, "counter") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		string sCounter = a_pArgv[++a_nIndex];
		if (sCounter.size() != 32 || sCounter.find_first_not_of("0123456789ABCDEFabcdef") != string::npos)
		{
			return kParseOptionReturnUnknownArgument;
		}
		m_Counter = sCounter.c_str();
		m_bCounterValid = true;
	}
	else if (strcmp(a_pName, "xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "compress-align") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		string sCompressAlign = a_pArgv[++a_nIndex];
		n32 nCompressAlign = SToN32(sCompressAlign);
		if (nCompressAlign != 1 && nCompressAlign != 4 && nCompressAlign != 8 && nCompressAlign != 16 && nCompressAlign != 32)
		{
			m_sMessage = sCompressAlign;
			return kParseOptionReturnUnknownArgument;
		}
		m_nCompressAlign = nCompressAlign;
	}
	else if (strcmp(a_pName, "compress-type") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		string sType = a_pArgv[++a_nIndex];
		if (sType == "blz")
		{
			m_eCompressType = kCompressTypeBlz;
		}
		else if (sType == "lz")
		{
			m_eCompressType = kCompressTypeLz;
		}
		else if (sType == "lzex")
		{
			m_eCompressType = kCompressTypeLzEx;
		}
		else if (sType == "h4")
		{
			m_eCompressType = kCompressTypeH4;
		}
		else if (sType == "h8")
		{
			m_eCompressType = kCompressTypeH8;
		}
		else if (sType == "rl")
		{
			m_eCompressType = kCompressTypeRl;
		}
		else if (sType == "yaz0")
		{
			m_eCompressType = kCompressTypeYaz0;
		}
		else
		{
			m_sMessage = sType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "compress-out") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sCompressOutFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "yaz0-align") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		string sYaz0Align = a_pArgv[++a_nIndex];
		n32 nYaz0Align = SToN32(sYaz0Align);
		if (nYaz0Align != 0 && nYaz0Align != 128)
		{
			m_sMessage = sYaz0Align;
			return kParseOptionReturnUnknownArgument;
		}
		m_nYaz0Align = nYaz0Align;
	}
	else if (strcmp(a_pName, "old") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sOldFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "new") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sNewFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "patch-file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sPatchFileName = a_pArgv[++a_nIndex];
	}
	else if (StartWith<string>(a_pName, "partition"))
	{
		int nIndex = SToN32(a_pName + strlen("partition"));
		if (nIndex < 0 || nIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_mNcchFileName[nIndex] = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "not-pad") == 0)
	{
		m_bNotPad = true;
	}
	else if (strcmp(a_pName, "trim-after-partition") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_nLastPartitionIndex = SToN32(a_pArgv[++a_nIndex]);
		if (m_nLastPartitionIndex < 0 || m_nLastPartitionIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
	}
	else if (strcmp(a_pName, "not-update-extendedheader-hash") == 0 || strcmp(a_pName, "not-update-exh-hash") == 0)
	{
		m_bNotUpdateExtendedHeaderHash = true;
	}
	else if (strcmp(a_pName, "not-update-exefs-hash") == 0)
	{
		m_bNotUpdateExeFsHash = true;
	}
	else if (strcmp(a_pName, "not-update-romfs-hash") == 0)
	{
		m_bNotUpdateRomFsHash = true;
	}
	else if (strcmp(a_pName, "extendedheader") == 0 || strcmp(a_pName, "exh") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sExtendedHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "logoregion") == 0 || strcmp(a_pName, "logo") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sLogoRegionFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "plainregion") == 0 || strcmp(a_pName, "plain") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sPlainRegionFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sExeFsFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sRomFsFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "extendedheader-xor") == 0 || strcmp(a_pName, "exh-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sExtendedHeaderXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sExeFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs-top-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (m_bExeFsTopAutoKey)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sExeFsTopXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (m_bRomFsAutoKey)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sRomFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs-top-auto-key") == 0)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (!m_sExeFsTopXorFileName.empty())
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bExeFsTopAutoKey = true;
	}
	else if (strcmp(a_pName, "romfs-auto-key") == 0)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		if (!m_sRomFsXorFileName.empty())
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bRomFsAutoKey = true;
	}
	else if (strcmp(a_pName, "exefs-dir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sExeFsDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs-dir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sRomFsDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "banner-dir") == 0)
	{
		if (a_nIndex + 1 > a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sBannerDirName = a_pArgv[++a_nIndex];
	}
	return kParseOptionReturnSuccess;
}

C3dsTool::EParseOptionReturn C3dsTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, char* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool C3dsTool::checkFileType()
{
	if (m_eFileType == kFileTypeUnknown)
	{
		if (CNcsd::IsNcsdFile(m_pFileName))
		{
			m_eFileType = kFileTypeCci;
		}
		else if (CNcch::IsCxiFile(m_pFileName))
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (CNcch::IsCfaFile(m_pFileName))
		{
			m_eFileType = kFileTypeCfa;
		}
		else if (CExeFs::IsExeFsFile(m_pFileName, 0))
		{
			m_eFileType = kFileTypeExeFs;
		}
		else if (CRomFs::IsRomFsFile(m_pFileName))
		{
			m_eFileType = kFileTypeRomFs;
		}
		else if (CBanner::IsBannerFile(m_pFileName))
		{
			m_eFileType = kFileTypeBanner;
		}
		else
		{
			m_sMessage = "unknown file type";
			return false;
		}
	}
	else
	{
		bool bMatch = false;
		switch (m_eFileType)
		{
		case kFileTypeCci:
			bMatch = CNcsd::IsNcsdFile(m_pFileName);
			break;
		case kFileTypeCxi:
			bMatch = CNcch::IsCxiFile(m_pFileName);
			break;
		case kFileTypeCfa:
			bMatch = CNcch::IsCfaFile(m_pFileName);
			break;
		case kFileTypeExeFs:
			bMatch = CExeFs::IsExeFsFile(m_pFileName, 0);
			break;
		case kFileTypeRomFs:
			bMatch = CRomFs::IsRomFsFile(m_pFileName);
			break;
		case kFileTypeBanner:
			bMatch = CBanner::IsBannerFile(m_pFileName);
			break;
		default:
			break;
		}
		if (!bMatch)
		{
			m_sMessage = "the file type is mismatch";
			return false;
		}
	}
	return true;
}

bool C3dsTool::extractFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case kFileTypeCci:
		{
			CNcsd ncsd;
			ncsd.SetFileName(m_pFileName);
			ncsd.SetVerbose(m_bVerbose);
			ncsd.SetHeaderFileName(m_pHeaderFileName);
			ncsd.SetNcchFileName(m_mNcchFileName);
			bResult = ncsd.ExtractFile();
		}
		break;
	case kFileTypeCxi:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetExtendedHeaderFileName(m_sExtendedHeaderFileName);
			ncch.SetLogoRegionFileName(m_sLogoRegionFileName);
			ncch.SetPlainRegionFileName(m_sPlainRegionFileName);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExtendedHeaderXorFileName(m_sExtendedHeaderXorFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetExeFsTopXorFileName(m_sExeFsTopXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetExeFsTopAutoKey(m_bExeFsTopAutoKey);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.ExtractFile();
		}
		break;
	case kFileTypeCfa:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.ExtractFile();
		}
		break;
	case kFileTypeExeFs:
		{
			CExeFs exeFs;
			exeFs.SetFileName(m_pFileName);
			exeFs.SetVerbose(m_bVerbose);
			exeFs.SetHeaderFileName(m_pHeaderFileName);
			exeFs.SetExeFsDirName(m_sExeFsDirName);
			exeFs.SetUncompress(m_bUncompress);
			bResult = exeFs.ExtractFile();
		}
		break;
	case kFileTypeRomFs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_pFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.SetRomFsDirName(m_sRomFsDirName);
			bResult = romFs.ExtractFile();
		}
		break;
	case kFileTypeBanner:
		{
			CBanner banner;
			banner.SetFileName(m_pFileName);
			banner.SetVerbose(m_bVerbose);
			banner.SetBannerDirName(m_sBannerDirName);
			bResult = banner.ExtractFile();
		}
		break;
	default:
		break;
	}
	return bResult;
}

bool C3dsTool::createFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case kFileTypeCci:
		{
			CNcsd ncsd;
			ncsd.SetFileName(m_pFileName);
			ncsd.SetVerbose(m_bVerbose);
			ncsd.SetHeaderFileName(m_pHeaderFileName);
			ncsd.SetNcchFileName(m_mNcchFileName);
			ncsd.SetNotPad(m_bNotPad);
			bResult = ncsd.CreateFile();
		}
		break;
	case kFileTypeCxi:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetNotUpdateExtendedHeaderHash(m_bNotUpdateExtendedHeaderHash);
			ncch.SetNotUpdateExeFsHash(m_bNotUpdateExeFsHash);
			ncch.SetNotUpdateRomFsHash(m_bNotUpdateRomFsHash);
			ncch.SetExtendedHeaderFileName(m_sExtendedHeaderFileName);
			ncch.SetLogoRegionFileName(m_sLogoRegionFileName);
			ncch.SetPlainRegionFileName(m_sPlainRegionFileName);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExtendedHeaderXorFileName(m_sExtendedHeaderXorFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetExeFsTopXorFileName(m_sExeFsTopXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetExeFsTopAutoKey(m_bExeFsTopAutoKey);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.CreateFile();
		}
		break;
	case kFileTypeCfa:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_Key);
			ncch.SetNotUpdateExeFsHash(m_bNotUpdateExeFsHash);
			ncch.SetNotUpdateRomFsHash(m_bNotUpdateRomFsHash);
			ncch.SetExeFsFileName(m_sExeFsFileName);
			ncch.SetRomFsFileName(m_sRomFsFileName);
			ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
			ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
			ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
			bResult = ncch.CreateFile();
		}
		break;
	case kFileTypeExeFs:
		{
			CExeFs exeFs;
			exeFs.SetFileName(m_pFileName);
			exeFs.SetVerbose(m_bVerbose);
			exeFs.SetHeaderFileName(m_pHeaderFileName);
			exeFs.SetExeFsDirName(m_sExeFsDirName);
			exeFs.SetCompress(m_bCompress);
			bResult = exeFs.CreateFile();
		}
		break;
	case kFileTypeRomFs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_pFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.SetRomFsDirName(m_sRomFsDirName);
			romFs.SetRomFsFileName(m_sRomFsFileName);
			bResult = romFs.CreateFile();
		}
		break;
	case kFileTypeBanner:
		{
			CBanner banner;
			banner.SetFileName(m_pFileName);
			banner.SetVerbose(m_bVerbose);
			banner.SetBannerDirName(m_sBannerDirName);
			bResult = banner.CreateFile();
		}
		break;
	default:
		break;
	}
	return bResult;
}

bool C3dsTool::encryptFile()
{
	bool bResult = false;
	if (m_nEncryptMode == CNcch::kEncryptModeAesCtr && m_bCounterValid)
	{
		bResult = FEncryptAesCtrFile(m_pFileName, m_Key, m_Counter, 0, 0, true, 0);
	}
	else if (m_nEncryptMode == CNcch::kEncryptModeXor && !m_sXorFileName.empty())
	{
		bResult = FEncryptXorFile(m_pFileName, m_sXorFileName, 0, 0, true, 0);
	}
	else if (CNcch::IsCxiFile(m_pFileName))
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetVerbose(m_bVerbose);
		ncch.SetEncryptMode(m_nEncryptMode);
		ncch.SetKey(m_Key);
		ncch.SetExtendedHeaderXorFileName(m_sExtendedHeaderXorFileName);
		ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
		ncch.SetExeFsTopXorFileName(m_sExeFsTopXorFileName);
		ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
		ncch.SetExeFsTopAutoKey(m_bExeFsTopAutoKey);
		ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
		bResult = ncch.EncryptFile();
	}
	else if (CNcch::IsCfaFile(m_pFileName))
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetVerbose(m_bVerbose);
		ncch.SetEncryptMode(m_nEncryptMode);
		ncch.SetKey(m_Key);
		ncch.SetExeFsXorFileName(m_sExeFsXorFileName);
		ncch.SetRomFsXorFileName(m_sRomFsXorFileName);
		ncch.SetRomFsAutoKey(m_bRomFsAutoKey);
		bResult = ncch.EncryptFile();
	}
	return bResult;
}

bool C3dsTool::uncompressFile()
{
	FILE* fp = Fopen(m_pFileName, "rb");
	bool bResult = fp != nullptr;
	if (bResult)
	{
		Fseek(fp, 0, SEEK_END);
		u32 uCompressedSize = static_cast<u32>(Ftell(fp));
		Fseek(fp, 0, SEEK_SET);
		u8* pCompressed = new u8[uCompressedSize];
		fread(pCompressed, 1, uCompressedSize, fp);
		fclose(fp);
		u32 uUncompressedSize = 0;
		switch (m_eCompressType)
		{
		case kCompressTypeBlz:
			bResult = CBackwardLz77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeLz:
		case kCompressTypeLzEx:
			bResult = CLz77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeH4:
		case kCompressTypeH8:
			bResult = CHuffman::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeRl:
			bResult = CRunLength::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case kCompressTypeYaz0:
			bResult = CYaz0::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		default:
			break;
		}
		if (bResult)
		{
			u8* pUncompressed = new u8[uUncompressedSize];
			switch (m_eCompressType)
			{
			case kCompressTypeBlz:
				bResult = CBackwardLz77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeLz:
			case kCompressTypeLzEx:
				bResult = CLz77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeH4:
			case kCompressTypeH8:
				bResult = CHuffman::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeRl:
				bResult = CRunLength::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case kCompressTypeYaz0:
				bResult = CYaz0::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			default:
				break;
			}
			if (bResult)
			{
				fp = Fopen(m_sCompressOutFileName.c_str(), "wb");
				bResult = fp != nullptr;
				if (bResult)
				{
					fwrite(pUncompressed, 1, uUncompressedSize, fp);
					fclose(fp);
				}
			}
			else
			{
				printf("ERROR: uncompress error\n\n");
			}
			delete[] pUncompressed;
		}
		else
		{
			printf("ERROR: get uncompressed size error\n\n");
		}
		delete[] pCompressed;
	}
	return bResult;
}

bool C3dsTool::compressFile()
{
	FILE* fp = Fopen(m_pFileName, "rb");
	bool bReuslt = fp != nullptr;
	if (bReuslt)
	{
		Fseek(fp, 0, SEEK_END);
		u32 uUncompressedSize = static_cast<u32>(Ftell(fp));
		Fseek(fp, 0, SEEK_SET);
		u8* pUncompressed = new u8[uUncompressedSize];
		fread(pUncompressed, 1, uUncompressedSize, fp);
		fclose(fp);
		u32 uCompressedSize = 0;
		switch (m_eCompressType)
		{
		case kCompressTypeBlz:
			uCompressedSize = uUncompressedSize;
			break;
		case kCompressTypeLz:
		case kCompressTypeLzEx:
			uCompressedSize = CLz77::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeH4:
		case kCompressTypeH8:
			uCompressedSize = CHuffman::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeRl:
			uCompressedSize = CRunLength::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeYaz0:
			uCompressedSize = CYaz0::GetCompressBoundSize(uUncompressedSize, m_nCompressAlign);
			break;
		default:
			break;
		}
		u8* pCompressed = new u8[uCompressedSize];
		switch (m_eCompressType)
		{
		case kCompressTypeBlz:
			bReuslt = CBackwardLz77::Compress(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize);
			break;
		case kCompressTypeLz:
			bReuslt = CLz77::CompressLz(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeLzEx:
			bReuslt = CLz77::CompressLzEx(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeH4:
			bReuslt = CHuffman::CompressH4(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeH8:
			bReuslt = CHuffman::CompressH8(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeRl:
			bReuslt = CRunLength::Compress(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign);
			break;
		case kCompressTypeYaz0:
			bReuslt = CYaz0::Compress(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize, m_nCompressAlign, m_nYaz0Align);
			break;
		default:
			break;
		}
		if (bReuslt)
		{
			fp = Fopen(m_sCompressOutFileName.c_str(), "wb");
			bReuslt = fp != nullptr;
			if (bReuslt)
			{
				fwrite(pCompressed, 1, uCompressedSize, fp);
				fclose(fp);
			}
		}
		else
		{
			printf("ERROR: compress error\n\n");
		}
		delete[] pCompressed;
		delete[] pUncompressed;
	}
	return bReuslt;
}

bool C3dsTool::trimFile()
{
	CNcsd ncsd;
	ncsd.SetFileName(m_pFileName);
	ncsd.SetVerbose(m_bVerbose);
	ncsd.SetLastPartitionIndex(m_nLastPartitionIndex);
	bool bResult = ncsd.TrimFile();
	return bResult;
}

bool C3dsTool::padFile()
{
	CNcsd ncsd;
	ncsd.SetFileName(m_pFileName);
	ncsd.SetVerbose(m_bVerbose);
	bool bResult = ncsd.PadFile();
	return bResult;
}

bool C3dsTool::diffFile()
{
	CPatch patch;
	patch.SetFileType(m_eFileType);
	patch.SetVerbose(m_bVerbose);
	patch.SetOldFileName(m_sOldFileName);
	patch.SetNewFileName(m_sNewFileName);
	patch.SetPatchFileName(m_sPatchFileName);
	return patch.CreatePatchFile();
}

bool C3dsTool::patchFile()
{
	CPatch patch;
	patch.SetFileName(m_pFileName);
	patch.SetVerbose(m_bVerbose);
	patch.SetPatchFileName(m_sPatchFileName);
	return patch.ApplyPatchFile();
}

int C3dsTool::sample()
{
	printf("sample:\n");
	printf("# extract cci\n");
	printf("3dstool -xvt017f cci 0.cxi 1.cfa 7.cfa input.3ds --header ncsdheader.bin\n\n");
	printf("# extract cxi without encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin\n\n");
	printf("# extract cxi with AES-CTR encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# extract cxi with xor encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n");
	printf("# extract cxi with 7.x xor encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-xor 000400000XXXXX00.0.exefs_top.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n");
	printf("# extract cxi with 7.x auto encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-auto-key --romfs-auto-key\n\n");
	printf("# extract cfa without encryption\n");
	printf("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin\n\n");
	printf("# extract cfa with AES-CTR encryption\n");
	printf("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# extract cfa with xor encryption\n");
	printf("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --romfs-xor 000400000XXXXX00.1.romfs.xor\n\n");
	printf("# extract exefs without Backward LZ77 uncompress\n");
	printf("3dstool -xvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# extract exefs with Backward LZ77 uncompress\n");
	printf("3dstool -xuvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# extract romfs\n");
	printf("3dstool -xvtf romfs romfs.bin --romfs-dir romfs\n\n");
	printf("# extract banner\n");
	printf("3dstool -xvtf banner banner.bnr --banner-dir banner\n\n");
	printf("# create cci with pad 0xFF\n");
	printf("3dstool -cvt017f cci 0.cxi 1.cfa 7.cfa output.3ds --header ncsdheader.bin\n\n");
	printf("# create cci without pad\n");
	printf("3dstool -cvt017f cci 0.cxi 1.cfa 7.cfa output.3ds --header ncsdheader.bin --not-pad\n\n");
	printf("# create cxi without encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --not-update-exh-hash --not-update-exefs-hash --not-update-romfs-hash\n\n");
	printf("# create cxi with AES-CTR encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# create cxi with xor encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n");
	printf("# create cxi with 7.x xor encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-xor 000400000XXXXX00.0.exefs_top.xor --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n");
	printf("# create cxi with 7.x auto encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.darc.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.0.exh.xor --exefs-xor 000400000XXXXX00.0.exefs.xor --exefs-top-auto-key --romfs-auto-key\n\n");
	printf("# create cfa without encryption and calculate hash\n");
	printf("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --not-update-romfs-hash\n\n");
	printf("# create cfa with AES-CTR encryption and calculate hash\n");
	printf("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# create cfa with xor encryption and calculate hash\n");
	printf("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --romfs-xor 000400000XXXXX00.0.romfs.xor\n\n");
	printf("# create exefs without Backward LZ77 compress\n");
	printf("3dstool -cvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# create exefs with Backward LZ77 compress\n");
	printf("3dstool -czvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# create romfs without reference\n");
	printf("3dstool -cvtf romfs romfs.bin --romfs-dir romfs\n\n");
	printf("# create romfs with reference\n");
	printf("3dstool -cvtf romfs romfs.bin --romfs-dir romfs --romfs original_romfs.bin\n\n");
	printf("# create banner\n");
	printf("3dstool -cvtf banner banner.bnr --banner-dir banner\n\n");
	printf("# encrypt file with AES-CTR encryption, standalone\n");
	printf("3dstool -evf file.bin --key 00000000000000000000000000000000 --counter 00000000000000000000000000000000\n\n");
	printf("# encrypt file with xor encryption, standalone\n");
	printf("3dstool -evf file.bin --xor xor.bin\n\n");
	printf("# uncompress file with Backward LZ77, standalone\n");
	printf("3dstool -uvf code.bin --compress-type blz --compress-out code.bin\n\n");
	printf("# compress file with Backward LZ77, standalone\n");
	printf("3dstool -zvf code.bin --compress-type blz --compress-out code.bin\n\n");
	printf("# uncompress file with LZ77, standalone\n");
	printf("3dstool -uvf input.lz --compress-type lz --compress-out output.bin\n\n");
	printf("# compress file with LZ77, standalone\n");
	printf("3dstool -zvf input.bin --compress-type lz --compress-out output.lz\n\n");
	printf("# compress file with LZ77 and align to 4 bytes, standalone\n");
	printf("3dstool -zvf input.bin --compress-type lz --compress-out output.lz --compress-align 4\n\n");
	printf("# uncompress file with LZ77Ex, standalone\n");
	printf("3dstool -uvf logo.darc.lz --compress-type lzex --compress-out logo.darc\n\n");
	printf("# compress file with LZ77Ex, standalone\n");
	printf("3dstool -zvf logo.darc --compress-type lzex --compress-out logo.darc.lz\n\n");
	printf("# uncompress file with Huffman 4bits, standalone\n");
	printf("3dstool -uvf input.bin --compress-type h4 --compress-out output.bin\n\n");
	printf("# compress file with Huffman 4bits, standalone\n");
	printf("3dstool -zvf input.bin --compress-type h4 --compress-out output.bin\n\n");
	printf("# uncompress file with Huffman 8bits, standalone\n");
	printf("3dstool -uvf input.bin --compress-type h8 --compress-out output.bin\n\n");
	printf("# compress file with Huffman 8bits, standalone\n");
	printf("3dstool -zvf input.bin --compress-type h8 --compress-out output.bin\n\n");
	printf("# uncompress file with RunLength, standalone\n");
	printf("3dstool -uvf input.bin --compress-type rl --compress-out output.bin\n\n");
	printf("# compress file with RunLength, standalone\n");
	printf("3dstool -zvf input.bin --compress-type rl --compress-out output.bin\n\n");
	printf("# uncompress file with Yaz0, standalone\n");
	printf("3dstool -uvf input.szs --compress-type yaz0 --compress-out output.sarc\n\n");
	printf("# compress file with Yaz0, standalone\n");
	printf("3dstool -zvf input.sarc --compress-type yaz0 --compress-out output.szs\n\n");
	printf("# compress file with Yaz0 and set the alignment property, standalone\n");
	printf("3dstool -zvf input.sarc --compress-type yaz0 --compress-out output.szs --yaz0-align 128\n\n");
	printf("# trim cci without pad\n");
	printf("3dstool --trim -vtf cci input.3ds\n\n");
	printf("# trim cci reserve partition 0~2\n");
	printf("3dstool --trim -vtf cci input.3ds --trim-after-partition 2\n\n");
	printf("# pad cci with 0xFF\n");
	printf("3dstool --pad -vtf cci input.3ds\n\n");
	printf("# create patch file without optimization\n");
	printf("3dstool --diff -v --old old.bin --new new.bin --patch-file patch.3ps\n\n");
	printf("# create patch file with cci optimization\n");
	printf("3dstool --diff -vt cci --old old.3ds --new new.3ds --patch-file patch.3ps\n\n");
	printf("# create patch file with cxi optimization\n");
	printf("3dstool --diff -vt cxi --old old.cxi --new new.cxi --patch-file patch.3ps\n\n");
	printf("# create patch file with cfa optimization\n");
	printf("3dstool --diff -vt cfa --old old.cfa --new new.cfa --patch-file patch.3ps\n\n");
	printf("# apply patch file\n");
	printf("3dstool --patch -vf input.bin --patch-file patch.3ps\n\n");
	return 0;
}

int main(int argc, char* argv[])
{
	SetLocale();
	C3dsTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
