#include<stdio.h>
#include"sensor.h"
#include<time.h>

void findPeaks(long long input);

void findRPeaks(long long peak);
void notRPeak(long long peak);
void correctInterval(long long peak);
void moveRecentRROk();
void moveRecentRR();
void calculateAverage();
void searchBack(long long peak2);

void dataOutput(long long peak, long long RR, long long place);

long long index = 0;

long long NPKF = 0;
long long SPKF = 0;
long long THRESHOLD1 = 0;
long long THRESHOLD2 = 0;

long long RR = 0;
long long olderIndex = 0;

long long RR_LOW = 0;
long long RR_HIGH = 0;
long long RR_MISS = 0;

long long RPEAKS[2000] = {0};
long long currentRIndex = 0;

long long RECENTRR_OK[8] = {0};
long long RECENTRR[8] = {0};

long long RR_AVERAGE1;
long long RR_AVERAGE2;

long long RR_ERROR = 0;

long long main()
{
	clock_t  start, end;
	double cpu_time_used;

	static const char filename[] = "ECG900K.txt";

	FILE *file = fopen(filename,"r");

	long long incoming;
	long long lowPassOut;
	long long highPassOut;
	long long deriveOut;
	long long squareOut;
	long long intSum;
	long long mWIOut;

	long long oldRaw[12] = {0};
	long long oldLowOut[32] = {0};
	long long oldHighOut[4] = {0};
	long long oldSquareOut[30] = {0};

	long long OSO_index = 0 ;
	long long raw_index = 0 ;


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

		intSum = 0;
		for(long long i = 29 ; i >= 1; i--){
			intSum += oldSquareOut[i] / 30;
		}
		mWIOut = intSum;

		//Updating the lists.
		for(long long i = 11 ; i >= 1 ; i--){
			oldRaw[i] = oldRaw[i-1];
		}

		for(long long i = 31 ; i >= 1 ; i--){
			oldLowOut[i] = oldLowOut[i-1];
		}


		for(long long i = 3 ; i >= 1 ; i--){
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
	cpu_time_used = cpu_time_used / 10000;

	printf("%f ms\n",cpu_time_used);

	return 0;
}

void findPeaks(long long input){

	static long long peakCheck[9] = {0};

	long long biggestLeft = 0;
	long long biggestRight = 0;

	for(long long i = 8 ; i >= 1 ; i--){
		peakCheck[i] = peakCheck[i - 1];
	}
	peakCheck[0] = input;


	for(long long i = 0 ; i < 4 ; i++){
		if(peakCheck[i] > biggestLeft){
			biggestLeft = peakCheck[i];
		}
	}

	for(long long i = 5 ; i < 9 ; i++){
		if(peakCheck[i] > biggestRight){
			biggestRight = peakCheck[i];
		}
	}

	if(peakCheck[4] >= biggestRight && peakCheck[4] > biggestLeft){

			findRPeaks(peakCheck[4]);

	}

}

void findRPeaks(long long peak){
	static long long PEAKS[2000] = {0};
	static long long INDEXES[2000] = {0};
	static long long PEAKINDEX = 0;


	PEAKS[PEAKINDEX] = peak;
	INDEXES[PEAKINDEX] = index - 4;
	PEAKINDEX++;

	if(peak <= THRESHOLD1){
		notRPeak(peak);
	}else{
		RR = (index - 4) - olderIndex;

		if(RR < RR_HIGH && RR > RR_LOW){
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

		if(RR > RR_MISS){

			long long tempIndex = PEAKINDEX -2;

			while(tempIndex >= 0 ){
				if(*(PEAKS + tempIndex) > THRESHOLD2){
					long long tempPeak = PEAKS[tempIndex];
					long long tempPeakIndex = INDEXES[tempIndex];
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

void notRPeak(long long peak){

	NPKF = (peak / 8) + (NPKF * 7 / 8);
	THRESHOLD1 = NPKF + ((SPKF - NPKF) / 4);
	THRESHOLD2 = THRESHOLD1 / 2;

	if((index - 4) - olderIndex >= 500){
		printf("WARNING: Heart failure!\n\n");
	}
}

void correctInterval(long long peak){
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
	for(long long i = 7 ; i > 0 ; i--){
		*(RECENTRR_OK + i) = *(RECENTRR_OK + i - 1);
	}
}

void moveRecentRR(){
	for(long long i = 7 ; i > 0 ; i--){
		*(RECENTRR + i) = *(RECENTRR + i - 1);
	}
}

void calculateAverage(){
	RR_AVERAGE1 = 0;
	RR_AVERAGE2 = 0;
	for(long long i = 7 ; i >= 0 ; i--){
		RR_AVERAGE2 += *(RECENTRR_OK + i);
		RR_AVERAGE1 += *(RECENTRR + i);
	}
	RR_AVERAGE1 = RR_AVERAGE1 / 8;
	RR_AVERAGE2 = RR_AVERAGE2 / 8;

}

void searchBack(long long peak2){
	moveRecentRR();
	*(RECENTRR) = RR;

	SPKF = (peak2 / 4) + (SPKF * 3 / 4);

	RR_AVERAGE1 = 0;
	for(long long i = 7 ; i >= 0 ; i--){
		RR_AVERAGE1 += *(RECENTRR + i)/8;
	}
	RR_AVERAGE1 = RR_AVERAGE1 / 8;

	RR_LOW = RR_AVERAGE1 * 92 / 100;
	RR_HIGH = RR_AVERAGE1 * 116 / 100;
	RR_MISS = RR_AVERAGE1 * 166 / 100;

	THRESHOLD1 = NPKF + (SPKF - NPKF) /4;
	THRESHOLD2 = THRESHOLD1 / 2;

}

void dataOutput(long long peak, long long RR, long long place){
	float time = place / 250.0;
	float pulse = 15000.0 / RR;

	if(peak < 2000){
		printf("R-Peak detected: %i, at time: %1.2f seconds.\nThe patients pulse: %1.2f beats per minutes. %i\n"
					"Warning: Beat to low!\n\n", peak, time, pulse,place );
	}


	printf("R-Peak detected: %i, at time: %1.2f seconds.\nThe patients pulse: %1.2f beats per minutes. %i\n"
					"\n", peak, time, pulse, place );



}
