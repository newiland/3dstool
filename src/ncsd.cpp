#include "ncsd.h"

const u32 CNcsd::s_uSignature = CONVERT_ENDIAN('NCSD');
const n64 CNcsd::s_nOffsetFirstNcch = 0x4000;
const int CNcsd::s_nBlockSize = 0x1000;

CNcsd::CNcsd()
	: m_nLastPartitionIndex(7)
	, m_pFileName(nullptr)
	, m_pHeaderFileName(nullptr)
	, m_bNotPad(false)
	, m_bVerbose(false)
	, m_fpNcsd(nullptr)
	, m_nMediaUnitSize(1 << 9)
	, m_nValidSize(0)
{
	memset(m_pNcchFileName, 0, sizeof(m_pNcchFileName));
	memset(&m_NcsdHeader, 0, sizeof(m_NcsdHeader));
	memset(&m_CardInfo, 0, sizeof(m_CardInfo));
}

CNcsd::~CNcsd()
{
}

void CNcsd::SetLastPartitionIndex(int a_nLastPartitionIndex)
{
	m_nLastPartitionIndex = a_nLastPartitionIndex;
}

void CNcsd::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CNcsd::SetHeaderFileName(const char* a_pHeaderFileName)
{
	m_pHeaderFileName = a_pHeaderFileName;
}

void CNcsd::SetNcchFileName(const char* a_pNcchFileName[])
{
	memcpy(m_pNcchFileName, a_pNcchFileName, sizeof(m_pNcchFileName));
}

void CNcsd::SetNotPad(bool a_bNotPad)
{
	m_bNotPad = a_bNotPad;
}

void CNcsd::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CNcsd::ExtractFile()
{
	bool bResult = true;
	m_fpNcsd = FFopen(m_pFileName, "rb");
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	calculateMediaUnitSize();
	if (!extractFile(m_pHeaderFileName, 0, s_nOffsetFirstNcch, "ncsd header", -1, false))
	{
		bResult = false;
	}
	for (int i = 0; i < 8; i++)
	{
		if (!extractFile(m_pNcchFileName[i], m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2], m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1], "partition", i, true))
		{
			bResult = false;
		}
	}
	fclose(m_fpNcsd);
	return bResult;
}

bool CNcsd::CreateFile()
{
	bool bResult = true;
	m_fpNcsd = FFopen(m_pFileName, "wb");
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	if (!createHeader())
	{
		fclose(m_fpNcsd);
		return false;
	}
	calculateMediaUnitSize();
	for (int i = 0; i < 8; i++)
	{
		if (!createNcch(i))
		{
			bResult = false;
		}
	}
	n64 nFileSize = FFtell(m_fpNcsd);
	*reinterpret_cast<n64*>(m_CardInfo.Reserved1 + 248) = nFileSize;
	if (m_bNotPad)
	{
		m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>(nFileSize / m_nMediaUnitSize);
	}
	else
	{
		m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>((1LL << 27) / m_nMediaUnitSize);
		for (int i = 27; i < 64; i++)
		{
			if (nFileSize <= 1LL << i)
			{
				m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>((1LL << i) / m_nMediaUnitSize);
				break;
			}
		}
		FPadFile(m_fpNcsd, m_NcsdHeader.Ncsd.MediaSize * m_nMediaUnitSize - nFileSize, 0xFF);
	}
	FFseek(m_fpNcsd, 0, SEEK_SET);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	fclose(m_fpNcsd);
	return bResult;
}

bool CNcsd::RipFile()
{
	m_fpNcsd = FFopen(m_pFileName, "rb+");
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fread(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	for (int i = m_nLastPartitionIndex + 1; i < 8; i++)
	{
		clearNcch(i);
	}
	calculateValidSize();
	*reinterpret_cast<n64*>(m_CardInfo.Reserved1 + 248) = m_nValidSize;
	m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>(m_nValidSize / m_nMediaUnitSize);
	FFseek(m_fpNcsd, 0, SEEK_SET);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	FChsize(FFileno(m_fpNcsd), m_nValidSize);
	fclose(m_fpNcsd);
	return true;
}

bool CNcsd::PadFile()
{
	m_fpNcsd = FFopen(m_pFileName, "rb+");
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fread(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	calculateValidSize();
	*reinterpret_cast<n64*>(m_CardInfo.Reserved1 + 248) = m_nValidSize;
	m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>((1LL << 27) / m_nMediaUnitSize);
	for (int i = 27; i < 64; i++)
	{
		if (m_nValidSize <= 1LL << i)
		{
			m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>((1LL << i) / m_nMediaUnitSize);
			break;
		}
	}
	FFseek(m_fpNcsd, m_nValidSize, SEEK_SET);
	FPadFile(m_fpNcsd, m_NcsdHeader.Ncsd.MediaSize * m_nMediaUnitSize - m_nValidSize, 0xFF);
	FFseek(m_fpNcsd, 0, SEEK_SET);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	FChsize(FFileno(m_fpNcsd), m_NcsdHeader.Ncsd.MediaSize * m_nMediaUnitSize);
	fclose(m_fpNcsd);
	return true;
}

bool CNcsd::IsNcsdFile(const char* a_pFileName)
{
	FILE* fp = FFopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	SNcsdHeader ncsdHeader;
	fread(&ncsdHeader, sizeof(ncsdHeader), 1, fp);
	fclose(fp);
	return ncsdHeader.Ncsd.Signature == s_uSignature;
}

void CNcsd::calculateMediaUnitSize()
{
	m_nMediaUnitSize = 1LL << (m_NcsdHeader.Ncsd.Flags[6] + 9);
}

void CNcsd::calculateValidSize()
{
	m_nValidSize = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[0] + m_NcsdHeader.Ncsd.ParitionOffsetAndSize[1];
	for (int i = 1; i < 8; i++)
	{
		n64 nSize = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2] + m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1];
		if (nSize > m_nValidSize)
		{
			m_nValidSize = nSize;
		}
	}
	m_nValidSize *= m_nMediaUnitSize;
}

bool CNcsd::extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, const char* a_pType, int a_nTypeId, bool bMediaUnitSize)
{
	bool bResult = true;
	if (a_pFileName != nullptr)
	{
		if (a_nOffset != 0 || a_nSize != 0)
		{
			FILE* fp = FFopen(a_pFileName, "wb");
			if (fp == nullptr)
			{
				bResult = false;
			}
			else
			{
				if (m_bVerbose)
				{
					printf("save: %s\n", a_pFileName);
				}
				if (bMediaUnitSize)
				{
					a_nOffset *= m_nMediaUnitSize;
					a_nSize *= m_nMediaUnitSize;
				}
				FCopyFile(fp, m_fpNcsd, a_nOffset, a_nSize);
				fclose(fp);
			}
		}
		else if (m_bVerbose)
		{
			if (a_nTypeId < 0 || a_nTypeId >= 8)
			{
				printf("INFO: %s is not exists, %s will not be create\n", a_pType, a_pFileName);
			}
			else
			{
				printf("INFO: %s %d is not exists, %s will not be create\n", a_pType, a_nTypeId, a_pFileName);
			}
		}
	}
	else if ((a_nOffset != 0 || a_nSize != 0) && m_bVerbose)
	{
		if (a_nTypeId < 0 || a_nTypeId >= 8)
		{
			printf("INFO: %s is not extract\n", a_pType);
		}
		else
		{
			printf("INFO: %s %d is not extract\n", a_pType, a_nTypeId);
		}
	}
	return bResult;
}

bool CNcsd::createHeader()
{
	FILE* fp = FFopen(m_pHeaderFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	FFseek(fp, 0, SEEK_END);
	n64 nFileSize = FFtell(fp);
	if (nFileSize < sizeof(m_NcsdHeader) + sizeof(m_CardInfo))
	{
		fclose(fp);
		printf("ERROR: ncsd header is too short\n\n");
		return false;
	}
	if (m_bVerbose)
	{
		printf("load: %s\n", m_pHeaderFileName);
	}
	FFseek(fp, 0, SEEK_SET);
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, fp);
	fread(&m_CardInfo, sizeof(m_CardInfo), 1, fp);
	fclose(fp);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	FPadFile(m_fpNcsd, s_nOffsetFirstNcch - FFtell(m_fpNcsd), 0xFF);
	return true;
}

bool CNcsd::createNcch(int a_nIndex)
{
	if (m_pNcchFileName[a_nIndex] != nullptr && a_nIndex <= m_nLastPartitionIndex)
	{
		FILE* fp = FFopen(m_pNcchFileName[a_nIndex], "rb");
		if (fp == nullptr)
		{
			clearNcch(a_nIndex);
			return false;
		}
		if (m_bVerbose)
		{
			printf("load: %s\n", m_pNcchFileName[a_nIndex]);
		}
		FFseek(fp, 0, SEEK_END);
		n64 nFileSize = FFtell(fp);
		if (a_nIndex == 0)
		{
			if (nFileSize < sizeof(SNcchHeader))
			{
				fclose(fp);
				clearNcch(a_nIndex);
				return false;
			}
			FFseek(fp, sizeof(SNcchHeader) - sizeof(NcchCommonHeaderStruct), SEEK_SET);
			fread(&m_CardInfo.NcchHeader, 1, sizeof(m_CardInfo.NcchHeader), fp);
		}
		FFseek(fp, 0, SEEK_SET);
		m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2] = static_cast<u32>(FFtell(m_fpNcsd) / m_nMediaUnitSize);
		m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2 + 1] = static_cast<u32>(FAlign(nFileSize, s_nBlockSize) / m_nMediaUnitSize);
		FCopyFile(m_fpNcsd, fp, 0, nFileSize);
		fclose(fp);
	}
	else
	{
		clearNcch(a_nIndex);
	}
	return true;
}

void CNcsd::clearNcch(int a_nIndex)
{
	m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2] = 0;
	m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2 + 1] = 0;
	memset(&m_NcsdHeader.Ncsd.PartitionId[a_nIndex], 0, sizeof(m_NcsdHeader.Ncsd.PartitionId[a_nIndex]));
	if (a_nIndex == 0)
	{
		memset(&m_CardInfo.NcchHeader, 0, sizeof(m_CardInfo.NcchHeader));
	}
}
