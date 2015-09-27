#include<stdio.h>
#include"sensor.h"
#include<time.h>

void findPeaks(int input);

void findRPeaks(int peak);
void notRPeak(int peak);
void correctInterval(int peak);
void moveRecentRROk();
void moveRecentRR();
void calculateAverage();
void searchBack(int peak2);

void dataOutput(int peak, int RR, int place);

long index = 0;

int NPKF = 0;
int SPKF = 0;
int THRESHOLD1 = 0;
int THRESHOLD2 = 0;

int RR = 0;
int olderIndex = 0;

int RR_LOW = 0;
int RR_HIGH = 0;
int RR_MISS = 0;

int RPEAKS[200000] = {0};
int currentRIndex = 0;

int RECENTRR_OK[8] = {0};
int RECENTRR[8] = {0};

int RR_AVERAGE1;
int RR_AVERAGE2;

int RR_ERROR = 0;

int main()
{
	clock_t  start, end;
	double cpu_time_used;

	static const char filename[] = "ECG10800K.txt";

	FILE *file = fopen(filename,"r");

	int incoming;
	int lowPassOut;
	int highPassOut;
	int deriveOut;
	unsigned long squareOut;
	unsigned long intSum;
	unsigned long mWIOut;

	int oldRaw[12] = {0};
	int oldLowOut[32] = {0};
	int oldHighOut[4] = {0};
	unsigned long oldSquareOut[30] = {0};

	int OSO_index = 0 ;
	int raw_index = 0 ;


	incoming = getNextData(file);

	while(!isFileEnded(file)){
		start = clock();
		//Low - Pass Filter
		lowPassOut = ( 2 * oldLowOut[0] ) - oldLowOut[1] + (incoming - ( 2 * oldRaw[5] ) + oldRaw[11]) / 32;

		//High - Pass Filter
		highPassOut = oldHighOut[0] - ( lowPassOut / 32 ) + oldLowOut[15] - oldLowOut[16] + ( oldLowOut[31] / 32 );

		//Derivative Filter
		deriveOut = (2 * highPassOut + oldHighOut[0] - oldHighOut[2] - 2 * oldHighOut[3]) / 8;

		//Squaring Filter
		squareOut = deriveOut * deriveOut;

		//Moving Window Integrator
		oldSquareOut[OSO_index] = squareOut;
		if(OSO_index == 29){
			OSO_index = 0;
		}else{
			OSO_index++;
		}
/*
		for(int i = 29 ; i >= 1 ; i--){
			oldSquareOut[i] = oldSquareOut[i - 1];
		}
		oldSquareOut[0] = squareOut;
*/
		intSum = 0;
		for(int i = 29 ; i >= 0; i--){
			intSum += oldSquareOut[i] / 30;
		}
		mWIOut = intSum;

		//Updating the lists.
		for(int i = 11 ; i >= 1 ; i--){
			oldRaw[i] = oldRaw[i-1];
		}

		for(int i = 31 ; i >= 1 ; i--){
			oldLowOut[i] = oldLowOut[i-1];
		}


		for(int i = 3 ; i >= 1 ; i--){
			oldHighOut[i] = oldHighOut[i-1];
		}

		oldRaw[0] = incoming;
		oldLowOut[0] = lowPassOut;
		oldHighOut[0] = highPassOut;

		end = clock();

		//Moving to next line.
		incoming = getNextData(file);


		//printf("Index: %i MWIOUt: %i\n",index,mWIOut);
		findPeaks(mWIOut);



		index ++;

		cpu_time_used += 1000.0 * ((double) (end - start)) / CLOCKS_PER_SEC;

	}
	cpu_time_used = cpu_time_used / 10800000;

	printf("%f ms\n",cpu_time_used);

	return 0;
}

void findPeaks(int input){

	static int peakCheck[9] = {0};

	int biggestLeft = 0;
	int biggestRight = 0;

	for(int i = 8 ; i >= 1 ; i--){
		peakCheck[i] = peakCheck[i - 1];
	}
	peakCheck[0] = input;


	for(int i = 0 ; i < 4 ; i++){
		if(peakCheck[i] > biggestLeft){
			biggestLeft = peakCheck[i];
		}
	}

	for(int i = 5 ; i < 9 ; i++){
		if(peakCheck[i] > biggestRight){
			biggestRight = peakCheck[i];
		}
	}

	if(peakCheck[4] >= biggestRight && peakCheck[4] > biggestLeft){

			findRPeaks(peakCheck[4]);

	}
}

void findRPeaks(int peak){
	static int PEAKS[200000] = {0};
	static int INDEXES[200000] = {0};
	static int PEAKINDEX = 0;


	PEAKS[PEAKINDEX] = peak;
	INDEXES[PEAKINDEX] = index - 4;
	PEAKINDEX++;

	if(peak <= THRESHOLD1){
		notRPeak(peak);
	}else{
		RR = (index - 4) - olderIndex;

		if(RR < RR_HIGH){
			if(RR > RR_LOW){

				correctInterval(peak);

				olderIndex = index - 4;

				RPEAKS[currentRIndex] = peak;
				currentRIndex++;
				dataOutput(peak, RR,index - 4);
				//printf("Index: %i - - - - Peak: %i - - - - MISS: %i - - - - RR: %i\n", index - 4, peak,RR_MISS,RR);
				//printf("%i\n",index - 4);
				RR_ERROR = 0;
			}else{
				RR_ERROR++;

				if(RR_ERROR >= 5){
					printf("Warning: Uneven beats!\n\n");
				}

			}
		}else if(RR > RR_MISS){

			int tempIndex = PEAKINDEX -2;

			while(tempIndex >= 0 ){

				if(PEAKS[tempIndex] > THRESHOLD2){

					int tempPeak = PEAKS[tempIndex];
					int tempPeakIndex = INDEXES[tempIndex];
					tempIndex = -1;

					RR = (index - 4) - tempPeakIndex;

					RPEAKS[currentRIndex] = tempPeak;
					currentRIndex++;

					searchBack(tempPeak);

					olderIndex = index - 4;
					dataOutput(tempPeak, RR,tempPeakIndex);
					//printf("Index: %i - - - - Peak: %i - - - - MISS: %i - - - - RR: %i\n", tempPeakIndex, tempPeak, RR_MISS, RR);
					//printf("%i\n",tempPeakIndex);
				}
				tempIndex--;
			}
		}

	}


}

void notRPeak(int peak){

	NPKF = (peak / 8) + (NPKF * 7 / 8);
	THRESHOLD1 = NPKF + ((SPKF - NPKF) / 4);
	THRESHOLD2 = THRESHOLD1 / 2;

	if((index - 4) - olderIndex >= 500){
		printf("WARNING: Heart failure!\n\n");
	}
}

void correctInterval(int peak){
	*(RPEAKS + currentRIndex) = peak;
	currentRIndex++ ;

	SPKF = (peak / 8) + (SPKF * 7 / 8);

	moveRecentRROk();
	*(RECENTRR_OK) = RR;

	moveRecentRR();
	*(RECENTRR) = RR;

	calculateAverage();

	RR_LOW = RR_AVERAGE2 * 92 / 100;
	RR_HIGH = RR_AVERAGE2 * 116 / 100;
	RR_MISS = RR_AVERAGE2 * 166 / 100;

	THRESHOLD1 = NPKF + ((SPKF - NPKF) / 4);
	THRESHOLD2 = THRESHOLD1 / 2;

}

void moveRecentRROk(){
	for(int i = 7 ; i > 0 ; i--){
		*(RECENTRR_OK + i) = *(RECENTRR_OK + i - 1);
	}
}

void moveRecentRR(){
	for(int i = 7 ; i > 0 ; i--){
		*(RECENTRR + i) = *(RECENTRR + i - 1);
	}
}

void calculateAverage(){
	RR_AVERAGE1 = 0;
	RR_AVERAGE2 = 0;
	for(int i = 7 ; i >= 0 ; i--){
		RR_AVERAGE2 += *(RECENTRR_OK + i);
		RR_AVERAGE1 += *(RECENTRR + i);
	}
	RR_AVERAGE1 = RR_AVERAGE1 / 8;
	RR_AVERAGE2 = RR_AVERAGE2 / 8;

}

void searchBack(int peak2){
	moveRecentRR();
	*(RECENTRR) = RR;

	SPKF = (peak2 / 4) + (SPKF * 3 / 4);

	RR_AVERAGE1 = 0;
	for(int i = 7 ; i >= 0 ; i--){
		RR_AVERAGE1 += *(RECENTRR + i)/8;
	}
	//RR_AVERAGE1 = RR_AVERAGE1 / 8;

	RR_LOW = RR_AVERAGE1 * 92 / 100;
	RR_HIGH = RR_AVERAGE1 * 116 / 100;
	RR_MISS = RR_AVERAGE1 * 166 / 100;

	THRESHOLD1 = NPKF + (SPKF - NPKF) /4;
	THRESHOLD2 = THRESHOLD1 / 2;

}

void dataOutput(int peak, int RR, int place){
	float time = place / 250.0;
	float pulse = 15000.0 / RR;

	if(peak < 2000){
		printf("R-Peak detected: %i, at time: %1.2f seconds.\nThe patients pulse: %1.2f beats per minutes. %i RR: %i LOW: %i High:%i MISS:%i\n"
					"Warning: Beat to low!\n\n", peak, time, pulse,place ,RR,RR_LOW,RR_HIGH,RR_MISS);
	}else{
		printf("R-Peak detected: %i, at time: %1.2f seconds.\nThe patients pulse: %1.2f beats per minutes. %i RR: %i LOW: %i High:%i MISS:%i\n"
					"\n", peak, time, pulse, place ,RR,RR_LOW,RR_HIGH,RR_MISS);
	}


}
