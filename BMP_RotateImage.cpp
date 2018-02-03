#include <malloc.h>
#include <stdio.h> 
#include <stdint.h>
#include <math.h>

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

enum colors
{
	RED = 0,
	GREEN,
	BLUE,

	END
};

#define M_PI 3.14159265358979323846

// ������� ���������� ���������(��������) ������ � ��������
int CountTrashPixels(int width)
{
	if ((width * 3) % 4 == 0)
	{
		return 0;
	}

	return 4 - (width * 3) % 4;
}

// ����� ������� � ������� ������� �������� (��������� ������ ������, ����)
int ChangeSizeImageAfterRotate(char* imgData, double angle, 
	double* xMin, double* yMin, 
	const fileStruck* fileStruckInputImage, 
	fileStruck* fileStruckOutputImage)
{
	int x0, y0;
	double xMax, yMax;

	*xMin = xMax = *yMin = yMax = 0; // ������ ���������

	//������������� �������������� �����
	for (int corner = 0; corner < 4; corner++)
	{
		x0 = ((*fileStruckInputImage).width - 1) * (corner % 2);
		y0 = ((*fileStruckInputImage).height - 1) * (corner / 2);

		double x = x0*cos(angle*M_PI / 180) - y0*sin(angle*M_PI / 180); // ���������� � ���� � ���������� �����������
		double y = x0*sin(angle*M_PI / 180) + y0*cos(angle*M_PI / 180); // ���������� � ���� � ���������� ����������� [�������� ������������ ������� ����������]

		if (x > xMax)	xMax = x;
		if (x < *xMin) *xMin = x;
		if (y > yMax)	yMax = y;
		if (y < *yMin) *yMin = y;
	}

	(*fileStruckOutputImage).width = xMax - *xMin + 1;	// ��������� ����� ������� ��������
	(*fileStruckOutputImage).height = yMax - *yMin + 1;	// � ������

	(*fileStruckOutputImage).massiveLength = (*fileStruckOutputImage).width * (*fileStruckOutputImage).height * 3; // ����� ������� ��������. ������*�����*3 - �.�. RGB.
	(*fileStruckOutputImage).massiveLength += (*fileStruckOutputImage).height * CountTrashPixels((*fileStruckOutputImage).width); // ��������� � ����� � �������� ����

	(*fileStruckOutputImage).fileSize = (*fileStruckOutputImage).massiveLength + (*fileStruckOutputImage).fileHeadSize; // ������� ������ ��������

	return (*fileStruckOutputImage).massiveLength; // �� �����: ����� ������� � ������� � ������ �� ��������
}

// ������� �������� (����������� ������ � ����� �������� � ������, � ���������� ����� ��������� ��������)
void Rotate(char* imgDataDst, char* imgDataSrc, double angle, 
	double xMin, double yMin, 
	fileStruck* fileStruckInputImage, 
	fileStruck* fileStruckOutputImage)
{
	int xDest, yDest;

	int trashSrc = CountTrashPixels((*fileStruckInputImage).width);
	int trashDest = CountTrashPixels((*fileStruckOutputImage).width);

	// ���������� ����� ��������� � ������ � ������ �� ��������� �����������
	for (int xSrc = 0, ySrc = 0;; xSrc++)
	{
		if (xSrc == (*fileStruckInputImage).width - trashSrc)
		{
			xSrc = 0;
			ySrc++;
		}		
		if (ySrc >= (*fileStruckInputImage).height) break;

		// ���������� ����� ���������
		xDest = xSrc*cos(angle*M_PI / 180) - ySrc*sin(angle*M_PI / 180) - xMin;	// xMin - �������� �� ��� oX
		yDest = xSrc*sin(angle*M_PI / 180) + ySrc*cos(angle*M_PI / 180) - yMin;	// yMin - �������� �� ��� oY

		for (int color = colors::RED; color < colors::END; color++)	// ��� ����������� ������ ������ RGB
		{
			imgDataDst[xDest * 3 + yDest*((*fileStruckOutputImage).width * 3 + trashDest) + color] =
				imgDataSrc[xSrc * 3 + ySrc*((*fileStruckInputImage).width * 3 + trashSrc) + color];
		}	
	} 
}

// ������� ��� ����� �������� �� �����
int ReadingInputImage(char** imgData, const char* nameImageInput, fileStruck* fileStruckInputImage)
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

	// �������� ������ ��� ������ ��������
	*imgData = (char*)malloc((*fileStruckInputImage).massiveLength);

	// ���������� ������ � ����������������� ������
	fread_s(*imgData, (*fileStruckInputImage).massiveLength, (*fileStruckInputImage).massiveLength, 1, InputFile);

	fclose(InputFile);

	return 0;
}

// ������� ��� ������ �������� � ����
void WriteOutputImage(char* imgData, const char* nameImageOutput, const fileStruck* fileStruckOutputImage)
{
	FILE *OutputFile;

	fopen_s(&OutputFile, nameImageOutput, "wb");
	fwrite(fileStruckOutputImage, sizeof(fileStruck), 1, OutputFile); // ���������� ������������ ��������� 
	fwrite(imgData, (*fileStruckOutputImage).massiveLength, 1, OutputFile); // ���������� ������
	fclose(OutputFile);
}

int main(int argc, char** argv)
{
	char *imgDataFromInput, *imgDataToOutput;
	double xMin, yMin; // �������� ����� �������� ������������ ������ ���������
	fileStruck FSInputImage, FSOutputImage;

	const char* nameImageInput = "in.bmp";
	const char* nameImageOutput = "out.bmp";

	double angle = 90; // � ��������

	int res = ReadingInputImage(&imgDataFromInput, nameImageInput, &FSInputImage);
	if (res != 0) return res;

	FSOutputImage = FSInputImage;	// ���������� �������� ��������� ��������� ����������� � ��������� ������� ��������
	size_t size = ChangeSizeImageAfterRotate(imgDataFromInput, angle, &xMin, &yMin, &FSInputImage, &FSOutputImage);		// ����� ����� ������� ������ ������� ��������
	imgDataToOutput = (char*)malloc(size);				// �������� ������ ��� ������ ������
	Rotate(imgDataToOutput, imgDataFromInput, angle, xMin, yMin, &FSInputImage, &FSOutputImage);	// ������� ��������

	free(imgDataFromInput);
	WriteOutputImage(imgDataToOutput, nameImageOutput, &FSOutputImage);
	free(imgDataToOutput);

	return 0;
}