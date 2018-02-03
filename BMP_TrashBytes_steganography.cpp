#include <stdio.h> 
#include <stdint.h>
#include <fstream>

#pragma pack(push, 1) 
struct fileStruck
{
	uint16_t signature;
	uint32_t fileSize;
	uint16_t foo1;
	uint16_t foo2;
	uint32_t fileHeadSize;
	uint32_t headerSize;
	uint32_t width;
	uint32_t height;
	uint16_t colorDimensions;
	uint16_t density;
	uint32_t zipType;
	uint32_t massiveLength;
	uint32_t horResolution;
	uint32_t verResolution;
	uint32_t colorsQuantity;
	uint32_t shadesQuantity;
};
#pragma pack(pop) 

int CountTrashPixels(int width)
{
	if ((width * 3) % 4 == 0)
	{
		return 0;
	}

	return 4 - (width * 3) % 4;
}

void Crypto(char* imgData, fileStruck& fileStruckImage, const bool crypto)
{
	const auto trash = CountTrashPixels(fileStruckImage.width);
	const int x = fileStruckImage.width - trash;

	char a = '\0';

	std::ifstream fileSrc;
	int count = 0;

	std::ofstream fileDst;

	if (crypto == true)
	{
		fileSrc.open("text.txt", std::ios::in | std::ios::binary | std::ios::ate);
		fileSrc.unsetf(std::ios::skipws);
		count = fileSrc.tellg();
		fileSrc.seekg(0, std::ios::beg);
	}
	else
	{
		fileDst.open("file.txt", std::ios::out | std::ios::binary);
	}

	for (int y = 0; y < fileStruckImage.height; y++)
	{	
		for (int i = 0; i < trash; i++)
		{
			if (crypto)
			{
				if (count == 0)
				{
					a = '\0';
				}
				else
				{
					fileSrc.get(a);
					count--;
				}

				imgData[(x + i) + y*(fileStruckImage.width * 3 + trash)] = a;
			}
			else
			{
				fileDst << imgData[(x + i) + y*(fileStruckImage.width * 3 + trash)];
			}
		}	
	}

	if (crypto)
	{
		fileSrc.close();
	}
	else
	{
		fileDst.close();
	}
}

int ReadingImage(char** imgData, const char* nameImageInput, fileStruck* fileStruckInputImage)
{
	FILE *InputFile;

	if (fopen_s(&InputFile, nameImageInput, "rb") != 0)
	{
		printf("File not found\n");
		return 1;
	}

	fread_s(fileStruckInputImage, sizeof(fileStruck), sizeof(fileStruck), 1, InputFile);
	if ((*fileStruckInputImage).signature != 19778 || (*fileStruckInputImage).density != 24)
	{
		printf("Wrong file type\n");
		return 1;
	}

	*imgData = (char*)malloc((*fileStruckInputImage).massiveLength);

	fread_s(*imgData, (*fileStruckInputImage).massiveLength, (*fileStruckInputImage).massiveLength, 1, InputFile);

	fclose(InputFile);

	return 0;
}

void WriteImage(const char* imgData, const char* nameImage, const fileStruck& fileStruckImage)
{
	FILE *OutputFile;

	fopen_s(&OutputFile, nameImage, "wb");
	fwrite(&fileStruckImage, sizeof(fileStruck), 1, OutputFile);
	fwrite(imgData, fileStruckImage.massiveLength, 1, OutputFile);
	fclose(OutputFile);
}

int main()
{
	char *imgData = nullptr;
	fileStruck FSImage;
	constexpr bool crypto = false; // crypto or decrypto
	char* nameImage;

	if (crypto)
	{
		nameImage = "out.bmp";
	}
	else
	{
		nameImage = "out.bmp";
	}

	if (crypto)
	{
		std::ifstream src("in.bmp", std::ios::in | std::ios::binary);
		std::ofstream dst(nameImage, std::ios::out | std::ios::binary);

		dst << src.rdbuf();

		src.close();
		dst.close();
	}

	int res = ReadingImage(&imgData, nameImage, &FSImage);
	if (res != 0) return res;

	Crypto(imgData, FSImage, crypto);

	if (crypto)
	{
		WriteImage(imgData, nameImage, FSImage);
	}

	if (imgData != nullptr)
		free(imgData);

	return 0;
}