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

// Рассчёт количества незанятых(мусорных) байтов у картинки
int CountTrashPixels(int width)
{
	if ((width * 3) % 4 == 0)
	{
		return 0;
	}

	return 4 - (width * 3) % 4;
}

// Длина массива с данными будущей картинки (начальный массив данных, угол)
int ChangeSizeImageAfterRotate(char* imgData, double angle, 
	double* xMin, double* yMin, 
	const fileStruck* fileStruckInputImage, 
	fileStruck* fileStruckOutputImage)
{
	int x0, y0;
	double xMax, yMax;

	*xMin = xMax = *yMin = yMax = 0; // Начало координат

	//Рассматриваем местоположение углов
	for (int corner = 0; corner < 4; corner++)
	{
		x0 = ((*fileStruckInputImage).width - 1) * (corner % 2);
		y0 = ((*fileStruckInputImage).height - 1) * (corner / 2);

		double x = x0*cos(angle*M_PI / 180) - y0*sin(angle*M_PI / 180); // Координата Х угла у повёрнутого изображения
		double y = x0*sin(angle*M_PI / 180) + y0*cos(angle*M_PI / 180); // Координата У угла у повёрнутого изображения [вращение относительно нулевой координаты]

		if (x > xMax)	xMax = x;
		if (x < *xMin) *xMin = x;
		if (y > yMax)	yMax = y;
		if (y < *yMin) *yMin = y;
	}

	(*fileStruckOutputImage).width = xMax - *xMin + 1;	// Добавляем длину будущей картинки
	(*fileStruckOutputImage).height = yMax - *yMin + 1;	// И ширину

	(*fileStruckOutputImage).massiveLength = (*fileStruckOutputImage).width * (*fileStruckOutputImage).height * 3; // Длина будущей картинки. Ширина*Длина*3 - т.к. RGB.
	(*fileStruckOutputImage).massiveLength += (*fileStruckOutputImage).height * CountTrashPixels((*fileStruckOutputImage).width); // Добавляем в длину и мусорные биты

	(*fileStruckOutputImage).fileSize = (*fileStruckOutputImage).massiveLength + (*fileStruckOutputImage).fileHeadSize; // Изменим размер картинки

	return (*fileStruckOutputImage).massiveLength; // На вывод: длина массива с данными в байтах на картинке
}

// Поворот картинки (Копирование данных с одной картинки в другую, с изменением места положения пикселей)
void Rotate(char* imgDataDst, char* imgDataSrc, double angle, 
	double xMin, double yMin, 
	fileStruck* fileStruckInputImage, 
	fileStruck* fileStruckOutputImage)
{
	int xDest, yDest;

	int trashSrc = CountTrashPixels((*fileStruckInputImage).width);
	int trashDest = CountTrashPixels((*fileStruckOutputImage).width);

	// Нахождение новых координат и запись в массив по найденным координатам
	for (int xSrc = 0, ySrc = 0;; xSrc++)
	{
		if (xSrc == (*fileStruckInputImage).width - trashSrc)
		{
			xSrc = 0;
			ySrc++;
		}		
		if (ySrc >= (*fileStruckInputImage).height) break;

		// Нахождение новых координат
		xDest = xSrc*cos(angle*M_PI / 180) - ySrc*sin(angle*M_PI / 180) - xMin;	// xMin - смещение по оси oX
		yDest = xSrc*sin(angle*M_PI / 180) + ySrc*cos(angle*M_PI / 180) - yMin;	// yMin - смещение по оси oY

		for (int color = colors::RED; color < colors::END; color++)	// для выставления подрят байтов RGB
		{
			imgDataDst[xDest * 3 + yDest*((*fileStruckOutputImage).width * 3 + trashDest) + color] =
				imgDataSrc[xSrc * 3 + ySrc*((*fileStruckInputImage).width * 3 + trashSrc) + color];
		}	
	} 
}

// Функция для ввода картинки из файла
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

	// Выделяем память под данные картинки
	*imgData = (char*)malloc((*fileStruckInputImage).massiveLength);

	// Записываем данные в зарезервированную память
	fread_s(*imgData, (*fileStruckInputImage).massiveLength, (*fileStruckInputImage).massiveLength, 1, InputFile);

	fclose(InputFile);

	return 0;
}

// Функция для вывода картинки в файл
void WriteOutputImage(char* imgData, const char* nameImageOutput, const fileStruck* fileStruckOutputImage)
{
	FILE *OutputFile;

	fopen_s(&OutputFile, nameImageOutput, "wb");
	fwrite(fileStruckOutputImage, sizeof(fileStruck), 1, OutputFile); // Записываем заголовочную структуру 
	fwrite(imgData, (*fileStruckOutputImage).massiveLength, 1, OutputFile); // Записываем данные
	fclose(OutputFile);
}

int main(int argc, char** argv)
{
	char *imgDataFromInput, *imgDataToOutput;
	double xMin, yMin; // смещение новой картинки относительно центра координат
	fileStruck FSInputImage, FSOutputImage;

	const char* nameImageInput = "in.bmp";
	const char* nameImageOutput = "out.bmp";

	double angle = 90; // в градусах

	int res = ReadingInputImage(&imgDataFromInput, nameImageInput, &FSInputImage);
	if (res != 0) return res;

	FSOutputImage = FSInputImage;	// Записываем значения структуры исходного изображения в структуру будущей картинки
	size_t size = ChangeSizeImageAfterRotate(imgDataFromInput, angle, &xMin, &yMin, &FSInputImage, &FSOutputImage);		// Найдём длину массива данных будущей картинки
	imgDataToOutput = (char*)malloc(size);				// Выделяем память под массив данных
	Rotate(imgDataToOutput, imgDataFromInput, angle, xMin, yMin, &FSInputImage, &FSOutputImage);	// Повернём картинку

	free(imgDataFromInput);
	WriteOutputImage(imgDataToOutput, nameImageOutput, &FSOutputImage);
	free(imgDataToOutput);

	return 0;
}