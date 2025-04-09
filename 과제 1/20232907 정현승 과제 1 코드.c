#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

//do_something
#define DATALEN 500
#define CHECKLEN 5
#define CHECKASSIGNLEN 1000
#define NUMOFGROUP 4

void freeset(void** p);

//부동소수형의 precision은 거리 계산 시 고려하였으나 오버플로는 일어나지 않는다고 보았다(오버플로가 일어날 만큼의 데이터를 사용하지 않는다)
typedef struct _data {
	//do_something
	//mean을 정의해야 해 bool 자료형을 사용할 수 없음, 같은 이유로 정수형 변수도 float 사용(원래 float형이었던 건 double형 사용)
	//bool 써야 하는 건 없으면 0, 있으면 0
	//단 이 데이터에 대해 0.000... 으로 내려할 때같이 매우 작은 수에서의 precision은 전혀 고려하지 않음
	//(소수 몇번째 자리까지만 알면 되기 때문에 precision이 깊어봐야 의미없다)
	float batteryPower; //mAh, 501~1998
	float hasBluetooth;
	double clockSpeed; //0.5~3
	float supportDualSim;
	double frontCamPixel; //0~19
	float support4G;
	double internalMemory; //GB, 2~64
	double depth; //cm, 0.1~1
	float weight; //80~200
	double cores; //1~8
	double camPixel; //mega pixels, 0~20
	float pixelHeight; //0~1960
	float pixelWidth; //500~1998
	float ram; //MB, 256~3998
	double screenHeight; //cm, 5~19
	double screenWidth; //cm, 0~18
	double batteryLife; //2~20
	float support3G;
	float hasTouchscreen;
	float hasWifi;
} Data;

typedef struct _read_data {
	Data* readData;
	char* readStrings;
	size_t datalen;
} ReadData; //파일에서 데이처를 읽고 반환받을 값 모음

typedef struct _assigned_data {
	Data* data; //데이터
	Data* point; //할당된 점
} Assign;

typedef struct _k_means_medoids_result {
	Assign* assignResult;
	Data* allocdResultPoints;
} KMeansMedoidsResult; //k_means_medoids 계산 후 반환받을 값 모음(연산 결과, K-means면 동적 할당받은 Point들까지)

enum mode {
	K_means, K_medoids
};

int compare_k_means_medoids_result(const KMeansMedoidsResult* const p1, const KMeansMedoidsResult* const p2);
KMeansMedoidsResult k_means_medoids(const Data* const data, const size_t datasize, const int numberOfCenters, const enum mode mode);
int compareAssignsByAssignPointPointer(const Assign* const p1, const Assign* const p2);
double evaluateResult(const Assign* const result, const size_t datasize, const int numberOfCenters);
ReadData readData(const char* const Filename);
void writeData(const char* const Filename, const Assign* const result, const size_t datalen);
Data* makeRandomPoints(const int numberOfCenters);
Data reassignPoint_mean(const Assign* data, const size_t datasize);
Data reassignPoint_medoid(const Assign* data, const size_t datasize);
double sumDouble(double* doubles, size_t len);
void printResult(const Assign* const result, const size_t datalen);
void printData(const Data* const data);
void printPointData(const Data* const point);
double distanceBetweenData(const Data* const p1, const Data* const p2);
int compareDouble(const double* const p1, const double* const p2);

void main() {
	KMeansMedoidsResult assignResult[CHECKASSIGNLEN] = { 0, };
	KMeansMedoidsResult* sameResultStart = NULL, * p = NULL, * finalResult = NULL;
	int maxSameResult = 0;
	ReadData data = readData("초기 조건.csv");
	srand(time(NULL));
	for (p = assignResult; p < assignResult + sizeof(assignResult) / sizeof(assignResult[0]); p++) {
		*p = k_means_medoids(data.readData, data.datalen, NUMOFGROUP, K_means);
	}

	qsort(assignResult, sizeof(assignResult) / sizeof(assignResult[0]), sizeof(assignResult[0]), compare_k_means_medoids_result);
	sameResultStart = assignResult;
	for (p = assignResult + 1; p < assignResult + sizeof(assignResult) / sizeof(assignResult[0]); p++) {
		if (compare_k_means_medoids_result(p - 1, p)) { //기존과 결과가 다르면
			if (p - sameResultStart > maxSameResult) {
				finalResult = sameResultStart;
				maxSameResult = p - sameResultStart;
			}
			sameResultStart = p;
		}
	}
	if (p - sameResultStart > maxSameResult) {
		finalResult = sameResultStart;
		maxSameResult = p - sameResultStart;
	}
	printf("total result with %.1f%% rate of same result\n", (double) maxSameResult / (sizeof(assignResult) / sizeof(assignResult[0])) * 100);
	printResult(finalResult->assignResult, data.datalen);
	writeData("20232907.csv", finalResult->assignResult, data.datalen);

	for (KMeansMedoidsResult* p = assignResult; p < assignResult + sizeof(assignResult) / sizeof(assignResult[0]); p++) {
		freeset(&p->assignResult);
		freeset(&p->allocdResultPoints);
	}

	freeset(&data.readStrings);
	freeset(&data.readData);
}

int compare_k_means_medoids_result(const KMeansMedoidsResult* const p1, const KMeansMedoidsResult* const p2) {
	//잘 돌아간 결과를 하나로 모으기 위함
	int tmp;
	Assign* assign1, * assign2;
	for (assign1 = p1->assignResult, assign2 = p2->assignResult;
		assign1 < p1->assignResult + DATALEN; assign1++, assign2++) {
		//datalen을 가져올 방법이 없음
		tmp = assign1->data - assign2->data; //어쨌든 data는 하나를 돌려쓰기 때문에 포인터 값만 확인해도 문제없음
		if (tmp) return tmp;
		tmp = memcmp(assign1->point, assign2->point, sizeof(Data)); //point는 각각이 따로따로 저장되어 있기 때문에 그 값을 확인해주어야 함
		if (tmp) return tmp;
	}
	return 0;
}

KMeansMedoidsResult k_means_medoids(const Data* const data, const size_t datasize, const int numberOfCenters, const enum mode mode) {
	KMeansMedoidsResult result = { 0, };
	Data* resultPoints = makeRandomPoints(numberOfCenters); //결과 점
	Assign* assignResult[CHECKLEN] = { calloc(datasize * CHECKLEN, sizeof(Assign)) };
	result.assignResult = malloc(datasize * sizeof(Assign));
	if (resultPoints == NULL || assignResult == NULL || result.assignResult == NULL) {
		freeset(&resultPoints);
		freeset(assignResult);
		freeset(&result.assignResult);
		exit(1);
	}
	const Data (*reassignPoint[2])(const Assign*, const size_t) = { reassignPoint_mean, reassignPoint_medoid };
	int flagFinish = 0;
	Data* presultPoints = resultPoints;
	Assign* passignResult = assignResult[0];
	Assign* assignNextResult = NULL;
	double minimalDistance;
	Assign* assignPointsStart = NULL;
	int sameAssignPointsLength;

	for (const Data* p = data; p < data + datasize; p++, passignResult++) {
		passignResult->data = p; //assignResult에 data 넣는 과정
		passignResult->point = NULL;
	}
	for (Assign** p = assignResult + 1; p < assignResult + sizeof(assignResult) / sizeof(assignResult[0]); p++) {
		*p = *(p - 1) + datasize;
		passignResult = *p;
		for (const Data* pd = data; pd < data + datasize; pd++, passignResult++) {
			passignResult->data = pd; //assignResult에 data 넣는 과정
			passignResult->point = NULL;
		}
	}
	//assignResult[CHECKLEN][datasize] 만들기(alloc을 한 번만 하기 위함)
	//assignResult[0]: 지금 당장 assign 결과, assignResult[1]: 직전 assign 결과, ...
	//왔다갔다 하는 점이 있을 때 감지하기 위해 CHECKLEN을 2 이상으로 설정


	while (!flagFinish) {
		//Swap result
		assignNextResult = assignResult[sizeof(assignResult) / sizeof(assignResult[0]) - 1];
		for (int i = sizeof(assignResult) / sizeof(assignResult[0]) - 1; i; i--) {
			assignResult[i] = assignResult[i - 1];
		}
		assignResult[0] = assignNextResult; //assignResult 하나씩 당기기

		//Assignment Step
		for (passignResult = assignNextResult; passignResult < assignNextResult + datasize; passignResult++) {
			//가장 작은 거리의 resultPoint 구하기
			//우선 첫번째 resultPoint의 값을 넣음
			minimalDistance = distanceBetweenData(passignResult->data, resultPoints);
			passignResult->point = resultPoints;
			for (presultPoints = resultPoints + 1; presultPoints < resultPoints + numberOfCenters; presultPoints++) {
				double distance = distanceBetweenData(passignResult->data, presultPoints);
				if (distance < minimalDistance) {
					minimalDistance = distance;
					passignResult->point = presultPoints;
				}
			}
		}

		//Refitting step
		qsort(assignNextResult, datasize, sizeof(Assign), compareAssignsByAssignPointPointer);
		//resultPoint 수만큼 resultPoint 찾는 것을 반복해야 하므로 미리 정렬을 해 놓는 게 더 빠름
		
		passignResult = assignNextResult;
		presultPoints = resultPoints;
		while (passignResult < assignNextResult + datasize) {
			assignPointsStart = passignResult++;
			sameAssignPointsLength = 1;
			while (passignResult < assignNextResult + datasize && passignResult->point == assignPointsStart->point) {
				passignResult++;
				sameAssignPointsLength++;
			}
			*assignPointsStart->point = reassignPoint[mode](assignPointsStart, sameAssignPointsLength);
			//mode가 K_means면 mean, K_medoids면 medoid 검색
			//assignPointsStart ~ data + sameAssignPointsLength 사이의 모든 data의 point는 같으니 point를 수정하는 작업은 필요없음
		}

		//Checking
		//기존과 같은 결과가 나와서 종료
		for (Assign** p = assignResult + 1; p < assignResult + sizeof(assignResult) / sizeof(assignResult[0]); p++)
			if (!memcmp(*p, assignResult[0], datasize * sizeof(Assign))) {
				flagFinish = p - assignResult;
				break;
			}
	}

	//하나가 왔다갔다하는 식이면(이 경우 결과 합산 시 각기 다른 결과로 처리되어 합산에 문제 발생)
	if (flagFinish != 1) {
		int resultToTake = 0;
		for (Assign** p = assignResult + 1; p < assignResult + flagFinish; p++)
			//compare, 결과가 어떻게 나오던 상관없고 순서가 다르더라도 결과가 일정하기만 하면 됨
			for (Assign* p1 = assignResult[resultToTake], *p2 = *p; p1 < *p + datasize; p1++, p2++)
				if (p1->data != p2->data || p1->point != p2->point) { //point 조건까지는 없어도 될 것 같기는 함
					if (p1->data > p2->data) //부등호 바뀌어도 문제없이 동작함
						resultToTake = p - assignResult;
					else if (p1->data == p2->data && p1->point > p2->point)
						resultToTake = p - assignResult;
					break;
				}
		if (resultToTake == 0); //필요한 작업 없음
		else {
			//다시 사용하지 않는 변수 사용, swap(단순히 덮어쓰면 동적할당 해제 과정에서 문제 발생)
			assignNextResult = assignResult[resultToTake];
			assignResult[resultToTake] = assignResult[0];
			assignResult[0] = assignNextResult;
			//이 기준으로 refitting만 해 주면 됨
		}
		//정렬은 되어 있으므로 정렬과정 필요 없음
		passignResult = assignNextResult;
		presultPoints = resultPoints;
		while (passignResult < assignNextResult + datasize) {
			assignPointsStart = passignResult++;
			sameAssignPointsLength = 1;
			while (passignResult < assignNextResult + datasize && passignResult->point == assignPointsStart->point) {
				passignResult++;
				sameAssignPointsLength++;
			}
			*assignPointsStart->point = reassignPoint[mode](assignPointsStart, sameAssignPointsLength);
			//mode가 K_means면 mean, K_medoids면 medoid 검색
			//assignPointsStart ~ data + sameAssignPointsLength 사이의 모든 data의 point는 같으니 point를 수정하는 작업은 필요없음
		}
	}

	//결과값 담기
	memcpy(result.assignResult, assignResult[0], datasize * sizeof(Assign));
	result.allocdResultPoints = resultPoints;
	
	//assignResult 할당 해제(포인터 값을 보고 가장 작은 값으로 해제)
	passignResult = assignResult[0];
	for (Assign** p = assignResult + 1; p < assignResult + sizeof(assignResult) / sizeof(assignResult[0]); p++) {
		if (passignResult > *p) passignResult = *p;
	}
	free(passignResult);

	return result;
}

int compareAssignsByAssignPointPointer(const Assign* const p1, const Assign* const p2) {
	if (p2->point != p1->point) return memcmp(p1->point, p2->point, sizeof(Data));
	//AssignPoint가 다르면 이것 기준 정렬
	//정렬 순서는 상관없고 내용이 동일한 데이터가 같은 순서로 정렬되기만 하면 됨
	//두 Point가 데이터는 같고 포인터가 다른 경우는 생각하지 않음(코드 잘못 짠 경우 등 해당)
	else return (int)(p1->data - p2->data); //데이터 정렬도 해줌
}

double evaluateResult(const Assign* const result, const size_t datasize, const int numberOfCenters) {
	double* intraDistances = NULL;
	double* interDistances = NULL;
	double* pdistances = NULL;
	Data* centers = NULL;
	Data* pcenters = NULL;
	const Assign* pResult = result;
	Assign* startPoint;
	double temp;
	Assign* psubstart;
	Assign* pi;
	Assign* pj;
	const int combination = numberOfCenters % 2 == 0 ?
		numberOfCenters / 2 * (numberOfCenters - 1) : (numberOfCenters - 1) / 2 * numberOfCenters;
		//numberOfCenters C 2
	//result는 정렬된 상태

	intraDistances = malloc(numberOfCenters * sizeof(double));
	interDistances = malloc(combination * sizeof(double));
	centers = malloc(numberOfCenters * sizeof(Data));
	if (interDistances == NULL || intraDistances == NULL || centers == NULL) {
		freeset(&intraDistances);
		freeset(&interDistances);
		freeset(&centers);
		exit(1);
	}
	pdistances = intraDistances;
	pcenters = centers;

	while (pResult < result + datasize) {
		*pdistances = distanceBetweenData(pResult->point, pResult->data);
		startPoint = pResult++;
		*pcenters = *startPoint->point;
		while (pResult < result + datasize && pResult->point == startPoint->point) {
			temp = distanceBetweenData(pcenters, pResult->data);
			if (temp > *pdistances) *pdistances = temp;
			pResult++;
		}
		pdistances++;
		pcenters++;
	}

	pdistances = interDistances;
	//그냥 하나하나 연산함
	pResult = result;
	for (int i = 0; i < numberOfCenters - 1; i++) {
		//같은 center를 가진 데이터 범위 구하기
			
		for (startPoint = pResult++;
			pResult < result + datasize && pResult->point == startPoint->point;
			pResult++);

		pj = pResult;
		for (int j = i + 1; j < numberOfCenters; j++) {

			*pdistances = distanceBetweenData(centers + i, centers + j); //기본값

			for (psubstart = pj++;
				pj < result + datasize && pj->point == psubstart->point; pj++)
				for (pi = startPoint; pi < pResult; pi++) {
					temp = distanceBetweenData(pi->data, pj->data);
					if (temp < *pdistances) *pdistances = temp;
				}
			pdistances++;
		}
	}
	free(centers);
	temp = sumDouble(intraDistances, numberOfCenters) / sumDouble(interDistances, combination);
	free(intraDistances);
	free(interDistances);
	return temp;
}

ReadData readData(const char* const Filename) {
	ReadData result = { 0, }; //Data
	FILE* file;
	char* filein; //파일 내용 전부 담는 포인터
	size_t filelen = 0; //파일 데이터 길이 + 1 저장하는 곳(동적할당시 사용)
	Data* resultData; //결과 데이터 담는 포인터
	char** datas; //파일별 내용 담는 포인터
	size_t datalen = 0; //데이터 갯수 다시 한 번 확인
	char* pin; //filein 커서
	char** pdatas; //datas 커서
	Data* presultData; //resultData 커서
	const char divider[2] = ","; //strtok에 사용될 나누는 기준

	file = fopen(Filename, "rt");
	if (file == NULL) exit(1);

	fseek(file, 0, SEEK_END);
	filelen = (size_t)ftell(file) + 1; //+1을 하지 않은 값은 사용조차 하지 않음
	fseek(file, 0, SEEK_SET);
	
	filein = malloc(filelen * sizeof(char));
	datas = malloc(DATALEN * sizeof(char*));
	if (datas == NULL || filein == NULL) {
		fclose(file);
		freeset(&filein);
		freeset(&datas);
		exit(1);
	}
	pin = filein;
	pdatas = datas;

	fgets(pin, filelen - (pin - filein), file); //맨 앞줄 읽어 버리기

	while (!feof(file)) {
		fgets(pin, filelen - (pin - filein), file);
		if (*pin == '\0') break; //이게 없으면 계속 빈 문자열을 읽음
		*pdatas++ = pin;
		pin += strlen(pin);
		if (*(pin - 1) = '\n') //삭제해도 정상동작
			*(pin - 1) = '\0'; //'\n' -> '\0', strtok
		datalen++;
	}
	fclose(file);

	resultData = malloc(datalen * sizeof(Data));
	if (resultData == NULL) {
		fclose(file);
		freeset(&resultData);
		freeset(&filein);
		freeset(&datas);
		exit(1);
	}
	presultData = resultData;

	for (pdatas = datas; pdatas < datas + datalen; pdatas++, presultData++) {
		//do_something
		presultData->batteryPower = atof(strtok(*pdatas, divider));
		presultData->hasBluetooth = atof(strtok(NULL, divider));
		presultData->clockSpeed = atof(strtok(NULL, divider));
		presultData->supportDualSim = atof(strtok(NULL, divider));
		presultData->frontCamPixel = atof(strtok(NULL, divider));
		presultData->support4G = atof(strtok(NULL, divider));
		presultData->internalMemory = atof(strtok(NULL, divider));
		presultData->depth = atof(strtok(NULL, divider));
		presultData->weight = atof(strtok(NULL, divider));
		presultData->cores = atof(strtok(NULL, divider));
		presultData->camPixel = atof(strtok(NULL, divider));
		presultData->pixelHeight = atof(strtok(NULL, divider));
		presultData->pixelWidth = atof(strtok(NULL, divider));
		presultData->ram = atof(strtok(NULL, divider));
		presultData->screenHeight = atof(strtok(NULL, divider));
		presultData->screenWidth = atof(strtok(NULL, divider));
		presultData->batteryLife = atof(strtok(NULL, divider));
		presultData->support3G = atof(strtok(NULL, divider));
		presultData->hasTouchscreen = atof(strtok(NULL, divider));
		presultData->hasWifi = atof(strtok(NULL, divider));
	}
	free(datas);
	result.readData = resultData;
	freeset(&filein);
	// result.readStrings = filein;
	result.datalen = datalen;
	return result;
}

void writeData(const char* const Filename, const Assign* const result, const size_t datalen) {
	FILE* file = fopen(Filename, "wtx");
	Assign* pResult = result;
	Assign* assignPointsStart = NULL;
	Assign* p = NULL;
	Data* object; //연산량을 줄이기 위해 출력 대상 object를 찍음
	size_t dataIndex = 0;
	int pointIndex = 0;
	if (file == NULL) exit(1);

	while (pResult < result + datalen) {
		for (assignPointsStart = pResult++;
			pResult < result + datalen && assignPointsStart->point == pResult->point;
			pResult++);
		object = assignPointsStart->point;
		pointIndex++;

		//do_something
		fprintf(file, ",Group,%d,datas assigned:,%d\n", pointIndex, (int)(pResult - assignPointsStart));
		fputs("datas,battery_power,blue,clock_speed,dual_sim,fc,four_g,int_memory,m_dep,mobile_wt,n_cores,pc,px_height,px_width,ram,sc_h,sc_w,talk_time,three_g,touch_screen,wifi\n"
			, file);
		fprintf(file,
			"center,%.2f,%.1f%%,%.3f,%.1f%%,%.2f,%.1f%%,%.2f,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f%%,%.1f%%,%.1f%%\n",
			object->batteryPower,
			object->hasBluetooth * 100,
			object->clockSpeed,
			object->supportDualSim * 100,
			object->frontCamPixel,
			object->support4G * 100,
			object->internalMemory,
			object->depth,
			object->weight,
			object->cores,
			object->camPixel,
			object->pixelHeight,
			object->pixelWidth,
			object->ram,
			object->screenHeight,
			object->screenWidth,
			object->batteryLife,
			object->support3G * 100,
			object->hasTouchscreen * 100,
			object->hasWifi * 100);
		for (p = assignPointsStart, dataIndex = 0; p < pResult; p++) {
			object = p->data;
			dataIndex++;
			//원래 값보다 1이 작아질 것을 우려해 %d가 아닌 %.0f로 출력
			fprintf(file,
				"data %d,%.0f,%d,%.1f,%d,%.0f,%d,%.0f,%.1f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%d,%d,%d\n",
				dataIndex,
				object->batteryPower,
				object->hasBluetooth == 1.0,
				object->clockSpeed,
				object->supportDualSim == 1.0,
				object->frontCamPixel,
				object->support4G == 1.0,
				object->internalMemory,
				object->depth,
				object->weight,
				object->cores,
				object->camPixel,
				object->pixelHeight,
				object->pixelWidth,
				object->ram,
				object->screenHeight,
				object->screenWidth,
				object->batteryLife,
				object->support3G == 1.0,
				object->hasTouchscreen == 1.0,
				object->hasWifi == 1.0);
		}
		fputc('\n', file);
	}
	fprintf(file, ",evaluation:,%f\n", evaluateResult(result, datalen, pointIndex));
	fclose(file);
}

Data* makeRandomPoints(const int numberOfCenters) {
	Data* randomPoints = calloc(numberOfCenters, sizeof(Data));
	if (randomPoints == NULL) exit(1);
	//do_something
	for (Data* p = randomPoints; p < randomPoints + numberOfCenters; p++) {
		//RANDMAX같은 건 확률의 균등이 중요한게 아니므로 보정하지 않음
		p->batteryPower = (float)(rand() % 1498 + 501);
		p->hasBluetooth = (float)(rand() % 2);
		p->clockSpeed = (double)(rand() % 26 + 5) / 10;
		p->supportDualSim = (float)(rand() % 2);
		p->frontCamPixel = (double)(rand() % 20);
		p->support4G = (float)(rand() % 2);
		p->internalMemory = (double)(rand() % 63 + 2);
		p->depth = (double)(rand() % 10 + 1) / 10;
		p->weight = (float)(rand() % 121 + 80);
		p->cores = (double)(rand() % 8 + 1);
		p->camPixel = (double)(rand() % 21);
		p->pixelHeight = (float)(rand() % 1961);
		p->pixelWidth = (float)(rand() % 1499 + 500);
		p->ram = (float)(rand() % 3743 + 256);
		p->screenHeight = (double)(rand() % 15 + 5);
		p->screenWidth = (double)(rand() % 19);
		p->batteryLife = (double)(rand() % 19 + 2);
		p->support3G = (float)(rand() % 2);
		p->hasTouchscreen = (float)(rand() % 2);
		p->hasWifi = (float)(rand() % 2);
	}
	return randomPoints;
}

Data reassignPoint_mean(const Assign* data, const size_t datasize) {
	//data~data+datasize 사이의 모든 data의 point는 같음
	//do_something
	Data meanPoint = { 0.0,0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,0.0,
		0.0,0.0,0.0,0.0,0.0 };

	for (const Assign* p = data; p < data + datasize; p++) {
		//do_something
		//get_mean
		//모든 데이터가 원래는 정수형 또는 소수점 한 자리 수이므로 더할 때는 precision 고려 없음
		meanPoint.batteryPower += p->data->batteryPower;
		meanPoint.hasBluetooth += p->data->hasBluetooth;
		meanPoint.clockSpeed += p->data->clockSpeed;
		meanPoint.supportDualSim += p->data->supportDualSim;
		meanPoint.frontCamPixel += p->data->frontCamPixel;
		meanPoint.support4G += p->data->support4G;
		meanPoint.internalMemory += p->data->internalMemory;
		meanPoint.depth += p->data->depth;
		meanPoint.weight += p->data->weight;
		meanPoint.cores += p->data->cores;
		meanPoint.camPixel += p->data->camPixel;
		meanPoint.pixelHeight += p->data->pixelHeight;
		meanPoint.pixelWidth += p->data->pixelWidth;
		meanPoint.ram += p->data->ram;
		meanPoint.screenHeight += p->data->screenHeight;
		meanPoint.screenWidth += p->data->screenWidth;
		meanPoint.batteryLife += p->data->batteryLife;
		meanPoint.support3G += p->data->support3G;
		meanPoint.hasTouchscreen += p->data->hasTouchscreen;
		meanPoint.hasWifi += p->data->hasWifi;
	}
	//do_something
	meanPoint.batteryPower /= datasize;
	meanPoint.hasBluetooth /= datasize;
	meanPoint.clockSpeed /= datasize;
	meanPoint.supportDualSim /= datasize;
	meanPoint.frontCamPixel /= datasize;
	meanPoint.support4G /= datasize;
	meanPoint.internalMemory /= datasize;
	meanPoint.depth /= datasize;
	meanPoint.weight /= datasize;
	meanPoint.cores /= datasize;
	meanPoint.camPixel /= datasize;
	meanPoint.pixelHeight /= datasize;
	meanPoint.pixelWidth /= datasize;
	meanPoint.ram /= datasize;
	meanPoint.screenHeight /= datasize;
	meanPoint.screenWidth /= datasize;
	meanPoint.batteryLife /= datasize;
	meanPoint.support3G /= datasize;
	meanPoint.hasTouchscreen /= datasize;
	meanPoint.hasWifi /= datasize;
	return meanPoint;
}

Data reassignPoint_medoid(const Assign* data, const size_t datasize) {
	//인자를 size_t로 받은 이유는 이후 계산 과정에서의 오버플로 방지

	//data~data+datasize 사이의 모든 data의 point는 같음
	size_t medoidPoint = 0;
	double medoidValue = 0.0;
	double temp;
	double** distanceTable;
	if (datasize == 0) exit(1);
	else if (datasize == 1) return *data->data; //예외처리
	distanceTable = malloc(datasize * sizeof(double*));
	if (distanceTable == NULL) exit(1);
	*distanceTable = malloc((datasize * (datasize + 1)) / 2 * sizeof(double)); //distances
		//(datasize+1) C 2
	if (*distanceTable == NULL) { //distances == NULL
		freeset(&distanceTable);
		exit(1);
	}
	distanceTable[1] = *distanceTable + datasize;
	for (int i = 2; i < datasize; i++) {
		distanceTable[i] = distanceTable[i - 1] + i - 1;
	}
	/*
				_____________
		(temp)	|XXXXXXXXXXX| (원래 total로 썼다가 필요없어져서 다른 배열에서 쓰던 걸 여기로 옮김)
		1		|X..........|
		2		|XX.........|
		3		|XXX........|
		4		|XXXX.......|
		5		|XXXXX......|
		6		|XXXXXX.....|
		7		|XXXXXXX....|
		8		|XXXXXXXX...|
		9		|XXXXXXXXX..|
		10		|XXXXXXXXXX.|
				 012345678910
				^모양 (위의 사례에서 datasize == 11)
		distanceTable[x][y] == x번째 데이터와 y번째 데이터와의 거리, 단 x > y
		distanceTable[0][y] == y번째 데이터와 다른 모든 데이터와의 총 거리
	*/
	for (int i = 1; i < datasize; i++) {
		for (int j = 0; j < i; j++) {
			distanceTable[i][j] = distanceBetweenData(data[i].data, data[j].data);
		}
	}
	//각 점에서부터의 거리 합을 구하고 그 최소값인 점을 찾는 코드
	//total 값을 구할 때는 double의 유효범위 고려 필요
#define getDistance(x, y) (((x)>(y))?distanceTable[x][y]:((x)!=(y))?distanceTable[y][x]:0.0)

	medoidPoint = 0;
	for (int j = 0; j < datasize; j++) {
		distanceTable[0][j] = getDistance(0, j);
	}
	medoidValue = sumDouble(distanceTable[0], datasize);
	for (int i = 1; i < datasize; i++) {
		for (int j = 0; j < datasize; j++) {
			distanceTable[0][j] = getDistance(i, j);
		}
		temp = sumDouble(distanceTable[0], datasize);
		if (temp < medoidValue) {
			medoidPoint = i;
			temp = medoidValue;
		}
	}

#undef getDistance

	free(*distanceTable); //distances
	free(distanceTable);

	return *data[medoidPoint].data;
}

double sumDouble(double* doubles, size_t len) {
	double* calcur = doubles; //계산 순서 커서
	double* putcur = doubles; //계산 결과가 들어갈 위치(삽입 정렬)
	double resultOfTwo = *doubles; //두 수 계산 결과
	qsort(doubles, len, sizeof(double), compareDouble);
	//작은 순서대로 놓고 작은 것부터 더해야 double의 유효범위를 지킬 수 있음
	while (calcur < doubles + len - 1) {
		resultOfTwo = *calcur + *(calcur + 1);
		calcur++;
		for (putcur = calcur + 1; putcur < doubles + len && *putcur < resultOfTwo; putcur++); //삽입 위치 찾기
		putcur--; //들어갈 위치로, 이 이전의 데이터를 한 칸씩 앞으로 밀어야 함
		for (double* p = calcur + 1; p <= putcur; p++) {
			*(p - 1) = *p;
		}
		*putcur = resultOfTwo;
	}
	return resultOfTwo;
}

void printResult(const Assign* const result, const size_t datalen) {
	//data는 정렬되어 있음
	const Assign* startPoint;
	const Assign* pResult = result;
	int numOfGroup = 0;
	while (pResult < result + datalen) {
		numOfGroup++;
		startPoint = pResult;
		printPointData(startPoint->point);
		while (pResult < result + datalen && pResult->point == startPoint->point) {
			printData(pResult->data);
			pResult++;
		}
		putchar('\n');
	}
	printf("evaluation: %f\n", evaluateResult(result, datalen, numOfGroup));
}

void printData(const Data* const data) {
	//do_something
	printf("%4.0f   |        %c|%6.1f  |      %c|%10.0f   |     %c|%3.0f   |%3.1f  |%3.0f   |%2.0f   |%5.0f   |%5.0f   |%5.0f   |%4.0f   |%3.0f   |%3.0f   |%5.0f   |     %c|     %c|     %c",
		data->batteryPower,
		data->hasBluetooth == 1.0 ? 'Y' : 'N',
		data->clockSpeed,
		data->supportDualSim == 1.0 ? 'Y' : 'N',
		data->frontCamPixel,
		data->support4G == 1.0 ? 'Y' : 'N',
		data->internalMemory,
		data->depth,
		data->weight,
		data->cores,
		data->camPixel,
		data->pixelHeight,
		data->pixelWidth,
		data->ram,
		data->screenHeight,
		data->screenWidth,
		data->batteryLife,
		data->support3G == 1.0 ? 'Y' : 'N',
		data->hasTouchscreen == 1.0 ? 'Y' : 'N',
		data->hasWifi == 1.0 ? 'Y' : 'N');
	putchar('\n');
}

void printPointData(const Data* const point) {
	//do_something
	puts("-------+---------+--------+-------+-------------+------+------+-----+------+-----+--------+--------+--------+-------+------+------+--------+------+------+------");
	puts("battPow|Bluetooth|clockSpd|DualSim|frontCamPixel|  4G  |Memory|depth|weight|cores|camPixel|pixelDwn|pixelAcr|    ram|scrDwn|scrAcr|battLife|  3G  | Touch| Wifi ");
	printf("%7.2f|   %5.1f%%|%8.3f| %5.1f%%|%13.2f|%5.1f%%|%6.2f|%5.3f|%6.2f|%5.2f|%8.2f|%8.2f|%8.2f|%7.2f|%6.2f|%6.2f|%8.2f|%5.1f%%|%5.1f%%|%5.1f%%",
		point->batteryPower,
		point->hasBluetooth * 100,
		point->clockSpeed,
		point->supportDualSim * 100,
		point->frontCamPixel,
		point->support4G * 100,
		point->internalMemory,
		point->depth,
		point->weight,
		point->cores,
		point->camPixel,
		point->pixelHeight,
		point->pixelWidth,
		point->ram,
		point->screenHeight,
		point->screenWidth,
		point->batteryLife,
		point->support3G * 100,
		point->hasTouchscreen * 100,
		point->hasWifi * 100);
	putchar('\n');
	puts("-------+---------+--------+-------+-------------+------+------+-----+------+-----+--------+--------+--------+-------+------+------+--------+------+------+------");
}

double distanceBetweenData(const Data* const p1, const Data* const p2) {
	double res = 0.0;
	//do_something
	double eachdist[20] = { 0, };

	//1: 단순표준화
	eachdist[0] = (p2->batteryPower - p1->batteryPower) / 439.0;
	eachdist[1] = (p2->hasBluetooth - p1->hasBluetooth) / 0.5;
	eachdist[2] = (p2->clockSpeed - p1->clockSpeed) / 0.82;
	eachdist[3] = (p2->supportDualSim - p1->supportDualSim) / 0.5;
	eachdist[4] = (p2->frontCamPixel - p1->frontCamPixel) / 4.34;
	eachdist[5] = (p2->support4G - p1->support4G) / 0.5;
	eachdist[6] = (p2->internalMemory - p1->internalMemory) / 18.1;
	eachdist[7] = (p2->depth - p1->depth) / 0.29;
	eachdist[8] = (p2->weight - p1->weight) / 35.4;
	eachdist[9] = (p2->cores - p1->cores) / 2.29;
	eachdist[10] = (p2->camPixel - p1->camPixel) / 6.06;
	eachdist[11] = (p2->pixelHeight - p1->pixelHeight) / 444.0;
	eachdist[12] = (p2->pixelWidth - p1->pixelWidth) / 432.0;
	eachdist[13] = (p2->ram - p1->ram) / 1080.0;
	eachdist[14] = (p2->screenHeight - p1->screenHeight) / 4.21;
	eachdist[15] = (p2->screenWidth - p1->screenWidth) / 4.36;
	eachdist[16] = (p2->batteryLife - p1->batteryLife) / 5.46;
	eachdist[17] = (p2->support3G - p1->support3G) / 0.43;
	eachdist[18] = (p2->hasTouchscreen - p1->hasTouchscreen) / 0.5;
	eachdist[19] = (p2->hasWifi - p1->hasWifi) / 0.5;
	/*2: 가중치 포함
	eachdist[0] = (p2->batteryPower - p1->batteryPower) / 439.0;
	eachdist[1] = (p2->hasBluetooth - p1->hasBluetooth) / 0.5;
	eachdist[2] = (p2->clockSpeed - p1->clockSpeed) / 0.82 * 5;
	eachdist[3] = (p2->supportDualSim - p1->supportDualSim) / 0.5;
	eachdist[4] = (p2->frontCamPixel - p1->frontCamPixel) / 4.34 * 10;
	eachdist[5] = (p2->support4G - p1->support4G) / 0.5;
	eachdist[6] = (p2->internalMemory - p1->internalMemory) / 18.1 * 5;
	eachdist[7] = (p2->depth - p1->depth) / 0.29;
	eachdist[8] = (p2->weight - p1->weight) / 35.4;
	eachdist[9] = (p2->cores - p1->cores) / 2.29 * 5;
	eachdist[10] = (p2->camPixel - p1->camPixel) / 6.06 * 10;
	eachdist[11] = (p2->pixelHeight - p1->pixelHeight) / 444.0*2;
	eachdist[12] = (p2->pixelWidth - p1->pixelWidth) / 432.0*2;
	eachdist[13] = (p2->ram - p1->ram) / 1080.0 * 5;
	eachdist[14] = (p2->screenHeight - p1->screenHeight) / 4.21*3;
	eachdist[15] = (p2->screenWidth - p1->screenWidth) / 4.36*3;
	eachdist[16] = (p2->batteryLife - p1->batteryLife) / 5.46;
	eachdist[17] = (p2->support3G - p1->support3G) / 0.43;
	eachdist[18] = (p2->hasTouchscreen - p1->hasTouchscreen) / 0.5;
	eachdist[19] = (p2->hasWifi - p1->hasWifi) / 0.5;
	*/

	for (double* p = eachdist; p < eachdist + sizeof(eachdist) / sizeof(eachdist[0]); p++)
		*p *= *p; //제곱
	
	res = sqrt(sumDouble(eachdist, sizeof(eachdist) / sizeof(eachdist[0])));

	return res;
}

int compareDouble(const double* const p1, const double* const p2) {
	if (*p1 > *p2) return 1;
	else if (*p1 < *p2) return -1;
	else return 0;
}

void freeset(void** p) {
	if (*p != NULL) {
		free(*p);
		*p = NULL;
	}
}