#include "audioBuffer.h"
#include <stdio.h>

void PrintArray(BYTE *pData,int nData)
{
	printf("A ");
	for (int i=0;i<nData;i++)
	{
		printf(" %2d  ",((unsigned char*)pData)[i]);
	}
	printf("\n\n");
}

void main_()
{
	AudioBuffer buffer(9);
	int count = 3;
	buffer.PrintBuffer();
	BYTE bufferData[4];
	BYTE bufferData2[9];
	int nSize;

	BYTE data1[] = {count++,count++,count++};
	buffer.PushBuffer(data1,sizeof(data1));
	buffer.PrintBuffer();
	nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	PrintArray(bufferData,nSize);
	
	

	BYTE data2[] = {count++,count++};
	buffer.PushBuffer(data2,sizeof(data2));
	buffer.PrintBuffer();
	//nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	//PrintArray(bufferData,nSize);
	printf("\n");
	


	BYTE data3[] = {count++,count++,count++,count++,count++,count++,count++,count++,count++,count++,count++,count++};
	printf("sizeof %d\n",sizeof(data3));
	buffer.PushBuffer(data3,sizeof(data3));
	buffer.PrintBuffer();
	nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	PrintArray(bufferData,nSize);
	


	BYTE data4[] = {count++,count++,count++,count++};
	buffer.PushBuffer(data4,sizeof(data4));
	buffer.PrintBuffer();
	nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	PrintArray(bufferData,nSize);
	
		nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	PrintArray(bufferData,nSize);

		nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	PrintArray(bufferData,nSize);

		nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	PrintArray(bufferData,nSize);

		nSize = buffer.GetBufferFront(bufferData,sizeof(bufferData));
	PrintArray(bufferData,nSize);
	printf("\n");


	for (int j=0;j<6;j++)
	{
		printf("loop %d\n",j);
		BYTE data5[] = {count++,count++,count++};
		buffer.PushBuffer(data5,sizeof(data5));
		printf("old ");buffer.PrintBuffer();
		nSize = buffer.GetBufferFront(bufferData2,sizeof(bufferData2));
		printf("new ");buffer.PrintBuffer();
		printf("    ");PrintArray(bufferData2,nSize);
	}
		 

	

}