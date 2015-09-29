#include<stdio.h>
#include"sensor.h"
#include<time.h>

void lowPassFilter(int input);
void highPassFilter();
void deriveOutAndSquaring();
void movingWindowIntegration();
void findPeaks( int input);

void findRPeaks( int peak);
void notRPeak( int peak);
void correctInterval( int peak);
void moveRecentRROk();
void moveRecentRR();
void calculateAverage();
void searchBack( int peak2);

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

int RPEAKS[2000000] = {0};
int currentRIndex = 0;

int RECENTRR_OK[8] = {0};
int RECENTRR[8] = {0};

int RR_AVERAGE1;
int RR_AVERAGE2;

int RR_ERROR = 0;

int incoming;
	 long lowPassOut;
	 long highPassOut;
	 long deriveOut;
	unsigned long squareOut;
	unsigned long intSum;
	unsigned long mWIOut;

	long oldRaw[12] = {0};
	 long oldLowOut[32] = {0};
	 long oldHighOut[4] = {0};
	unsigned long oldSquareOut[30] = {0};

	int OSO_index = 0 ;
	int raw_index = 0 ;

int AVERAGE1_DIVISOR = 0;
int AVERAGE2_DIVISOR = 0;
int MWI_DIVISOR;

int main()
{


	//for( int i = 0 ; i < 100 ; i++){

		clock_t  start, end,start_total,end_total,start_low,end_low,start_high,end_high,start_deriveSquare,end_deriveSquare,start_MWI,end_MWI,
					start_peak,end_peak;
				double cpu_time_used, max_time_used,time_used,total_time,total_time_max,low_time_used,low_time_max,high_time_used,high_time_max,derive_time,derive_time_max,
				mwi_time,mwi_time_max,peak_time,peak_time_max;
				int fileSize;

	RR_AVERAGE1 = 151;
	RR_AVERAGE2 = 151;
	RR_LOW = RR_AVERAGE2 / 92 * 100;
	RR_HIGH = RR_AVERAGE2 / 116 * 100;
	RR_MISS = RR_AVERAGE2 / 166 * 100;

	THRESHOLD1 = 1200;
	THRESHOLD2 = 600;

	SPKF = 4200;
	NPKF = 200;




	fileSize = 10000;

	static const char filename[] = "ECG.txt";

	FILE *file = fopen(filename,"r");

	incoming = getNextData(file);

	cpu_time_used = 0.0;

	MWI_DIVISOR = 0;

	start_total = clock();

	while(!isFileEnded(file)){
		time_used = 0;
		start = clock();
		//Low - Pass Filter
		start_low = clock();
		lowPassFilter(incoming);
		end_low = clock();

		if(1000.0 * ((double) end_low - start_low) / CLOCKS_PER_SEC > low_time_max){
			low_time_max = 1000.0 * ((double) end_low - start_low) / CLOCKS_PER_SEC;
			low_time_used += 1000.0 * ((double) end_low - start_low) / CLOCKS_PER_SEC;
		}else{
			low_time_used += 1000.0 * ((double) end_low - start_low) / CLOCKS_PER_SEC;
		}

		//High - Pass Filter
		start_high = clock();
		highPassFilter();
		end_high = clock();

		if(1000.0 * ((double) end_high - start_high) / CLOCKS_PER_SEC > high_time_max){
			high_time_max = 1000.0 * ((double) end_high - start_high) / CLOCKS_PER_SEC;
			high_time_used +=1000.0 * ((double) end_high - start_high) / CLOCKS_PER_SEC;
		}else{
			high_time_used +=1000.0 * ((double) end_high - start_high) / CLOCKS_PER_SEC;
		}

		//Derivative Filter
		start_deriveSquare = clock();
		deriveOutAndSquaring();
		end_deriveSquare = clock();

		if(1000.0 * ((double) end_deriveSquare - start_deriveSquare) / CLOCKS_PER_SEC > derive_time_max){
			derive_time_max = 1000.0 * ((double) end_deriveSquare - start_deriveSquare) / CLOCKS_PER_SEC;
			derive_time += 1000.0 * ((double) end_deriveSquare - start_deriveSquare) / CLOCKS_PER_SEC;
		}else{
			derive_time += 1000.0 * ((double) end_deriveSquare - start_deriveSquare) / CLOCKS_PER_SEC;
		}

		//Moving Window Integrator
		start_MWI = clock();
		movingWindowIntegration();
		end_MWI = clock();

		if(1000.0 * ((double) end_MWI - start_MWI) / CLOCKS_PER_SEC > mwi_time_max){
			mwi_time_max = 1000.0 * ((double) end_MWI - start_MWI) / CLOCKS_PER_SEC;
			mwi_time += 1000.0 * ((double) end_MWI - start_MWI) / CLOCKS_PER_SEC;
		}else{
			mwi_time += 1000.0 * ((double) end_MWI - start_MWI) / CLOCKS_PER_SEC;
		}

		intSum = 0;
		for(int i = 29 ; i >= 0; i--){
			intSum += oldSquareOut[i];
		}
		mWIOut = intSum / MWI_DIVISOR;

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

		printf("%i\n",mWIOut);
		start_peak = clock();
		findPeaks(mWIOut);
		end_peak = clock();
		if(1000.0 * ((double) end_peak - start_peak) / CLOCKS_PER_SEC > peak_time_max){
			peak_time_max = 1000.0 * ((double) end_peak - start_peak) / CLOCKS_PER_SEC;
			peak_time += 1000.0 * ((double) end_peak - start_peak) / CLOCKS_PER_SEC;
		}else{
			peak_time += 1000.0 * ((double) end_peak - start_peak) / CLOCKS_PER_SEC;
		}


		index ++;

		time_used = 1000.0 * ((double) (end - start)) / CLOCKS_PER_SEC;
		cpu_time_used += time_used;

		if(time_used > max_time_used){
			max_time_used = time_used;
		}


	}
	end_total = clock();
	if((1000.0 * ((double) (end_total - start_total)) / CLOCKS_PER_SEC) > total_time_max){
		total_time_max = 1000.0 * ((double) (end_total - start_total)) / CLOCKS_PER_SEC;
		total_time += (1000.0 * ((double) (end_total - start_total)) / CLOCKS_PER_SEC) /100;
	}else{

	}

	cpu_time_used = cpu_time_used / fileSize;

	low_time_used = low_time_used / fileSize;
	high_time_used = high_time_used / fileSize;
	derive_time = derive_time / fileSize;
	mwi_time = mwi_time / fileSize;
	peak_time = peak_time / fileSize;

/*
	printf("Total: %.4f ms ; Average: %.4f ms\n"
			"Filters: Maximum: %.4f ms ; Average: %.4f ms\n"
			"	Low: Maximum: %.4f ms ; Average: %.4f ms \n"
			"	High: Maximum: %.4f ms ; Average: %.4f ms \n"
			"	DeriveSquare: Maximum: %.4f ms ; Average: %.4f ms \n"
			"	MovingWindow: Maximum: %.4f ms ; Average: %.4f ms \n"
			"Peaks: Maximum: %.4f ms ; Average: %.4f ms\n",total_time_max, total_time,max_time_used,cpu_time_used,
			low_time_max,low_time_used,high_time_max,high_time_used,derive_time_max,derive_time,
			mwi_time_max,mwi_time,peak_time_max,peak_time);

	}
*/
	return 0;
}

void lowPassFilter(int input){
	lowPassOut = ( 2 * oldLowOut[0] ) - oldLowOut[1] + (input - ( 2 * oldRaw[5] ) + oldRaw[11]) / 32;
}

void highPassFilter(){
	highPassOut = oldHighOut[0] - ( lowPassOut / 32 ) + oldLowOut[15] - oldLowOut[16] + ( oldLowOut[31] / 32 );
}

void deriveOutAndSquaring(){

	deriveOut = (2 * highPassOut + oldHighOut[0] - oldHighOut[2] - 2 * oldHighOut[3]) / 8;

	//Squaring Filter
	squareOut = deriveOut * deriveOut;
}

void movingWindowIntegration(){
	oldSquareOut[OSO_index] = squareOut;
			if(OSO_index == 29){
				OSO_index = 0;
			}else{
				OSO_index++;
			}
			if(MWI_DIVISOR < 30){
				MWI_DIVISOR++;
			}
}

void findPeaks( int input){

	static  int peakCheck[19] = {0};

	 int biggestLeft = 0;
	 int biggestRight = 0;

	for(int i = 18 ; i >= 1 ; i--){
		peakCheck[i] = peakCheck[i - 1];
	}
	peakCheck[0] = input;


	for(int i = 0 ; i < 9 ; i++){
		if(peakCheck[i] > biggestLeft){
			biggestLeft = peakCheck[i];
		}
	}

	for(int i = 10 ; i < 19 ; i++){
		if(peakCheck[i] > biggestRight){
			biggestRight = peakCheck[i];
		}
	}

	if(peakCheck[9] >= biggestRight && peakCheck[9] > biggestLeft){

			findRPeaks(peakCheck[9]);

	}

}

void findRPeaks( int peak){
	static int PEAKS[2000000] = {0};
	static int INDEXES[2000000] = {0};
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
				RR_ERROR = 0;
			}else{
				RR_ERROR++;

				if(RR_ERROR >= 5){
					//printf("Warning: Uneven beats!\n\n");
				}

			}
		}

		if(RR > RR_MISS){

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

					olderIndex = tempPeakIndex;
					dataOutput(tempPeak, RR,tempPeakIndex);

				}
				tempIndex--;

			}
		}

	}


}

void notRPeak( int peak){

	NPKF = (peak / 8) + (NPKF * 7 / 8);
	THRESHOLD1 = NPKF + ((SPKF - NPKF) / 4);
	THRESHOLD2 = THRESHOLD1 / 2;

	if((index - 4) - olderIndex >= 500){
	//	printf("WARNING: Heart failure!\n\n");
	}
}

void correctInterval( int peak){
	RPEAKS[currentRIndex] = peak;
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
		RECENTRR_OK[i] = RECENTRR_OK[i-1];
	}
	if(AVERAGE2_DIVISOR != 8){
		AVERAGE2_DIVISOR++;
	}
}

void moveRecentRR(){
	for(int i = 7 ; i > 0 ; i--){
		RECENTRR[i] = RECENTRR[i-1];
	}
	if(AVERAGE1_DIVISOR != 8){
		AVERAGE1_DIVISOR++;
	}
}

void calculateAverage(){
	RR_AVERAGE1 = 0;
	RR_AVERAGE2 = 0;
	for(int i = 7 ; i >= 0 ; i--){
		RR_AVERAGE2 += RECENTRR_OK[i];
		RR_AVERAGE1 += RECENTRR[i];
	}
	RR_AVERAGE1 = RR_AVERAGE1 / AVERAGE1_DIVISOR;
	RR_AVERAGE2 = RR_AVERAGE2 / AVERAGE2_DIVISOR;

}

void searchBack( int peak2){
	moveRecentRR();
	*(RECENTRR) = RR;

	SPKF = (peak2 / 4) + (SPKF * 3 / 4);

	RR_AVERAGE1 = 0;
	for(int i = 7 ; i >= 0 ; i--){
		RR_AVERAGE1 += RECENTRR[i];
	}
	RR_AVERAGE1 = RR_AVERAGE1 / AVERAGE1_DIVISOR;

	RR_LOW = RR_AVERAGE1 * 92 / 100;
	RR_HIGH = RR_AVERAGE1 * 116 / 100;
	RR_MISS = RR_AVERAGE1 * 166 / 100;

	THRESHOLD1 = NPKF + ( (SPKF - NPKF) / 4 );
	THRESHOLD2 = THRESHOLD1 / 2;

}

void dataOutput(int peak, int RR, int place){
	float time = place / 250.0;
	float pulse = 15000.0 / RR;

	/*
	if(peak < 2000){
		printf("R-Peak detected: %i, at time: %1.2f seconds.\nThe patients pulse: %1.2f beats per minutes.\n"
					"Warning: Beat to low!\n\n", peak, time, pulse);
	}else{
		printf("R-Peak detected: %i, at time: %1.2f seconds.\nThe patients pulse: %1.2f beats per minutes.\n"
					"\n", peak, time,pulse);
	}
*/

}
