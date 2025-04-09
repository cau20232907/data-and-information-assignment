#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

//사람 키, 몸무게 정보
typedef struct _person {
	double height; //cm, mean=182.9, variance=92
	double weight; //kg, mean=67.7, variance=72
} Person;

//사람 배열과 데이터 크기
typedef struct _read_file {
	Person* data; //데이터 배열
	size_t dataLen; //데이터 크기
} ReadFile;

//각 데이터 지점과 확률 분포 값(과제에서 요구하는 그대로)
typedef struct _person_chance {
	Person data; //데이터
	//확률 분포 값
	double original;
	double MLEvalue;
	double squareParzen[3];
	double roundParzen[3];
	double squareKNN[3];
	double roundKNN[3];
	//오차
	double MLEvalueError;
	double squareParzenError[3];
	double roundParzenError[3];
	double squareKNNError[3];
	double roundKNNError[3];
} PersonChance;

//Gaussian 파라미터 정보(MLE에서 구한 값이나 원래 값 저장 용도)
typedef struct _gaussian_info {
	Person mean;
	double std[2][2];
} GaussianInfo;

//최종 결과
typedef struct _staticics_result {
	PersonChance* data;
	GaussianInfo MLEresult;
	GaussianInfo MLEError;
	size_t dataLen;
} StatResult;

//height 가중치
#define heightWeight 1
//weight 가중치
#define weightWeight 1
//파일 한 줄의 최대 길이(동적 할당을 받지 않기 위해 활용)
#define LINELEN 256
//구하는 데이터 지점의 수(재할당을 받지 않기 위해 활용)
#define DATALEN 1777

StatResult getFunctionDistribution(const ReadFile data, const GaussianInfo standard);
StatResult collectResult(const StatResult* const results, size_t len);
GaussianInfo MLE(const ReadFile data);
double getGaussianValueatPoint(const GaussianInfo gaussianInfo, const Person target);
double sumDouble(double* doubles, size_t len);
double squareParzenWindow(const ReadFile data, const Person target, const Person windowSize);
double roundParzenWindow(const ReadFile data, const Person target, double windowradius);
double* distancesFromTarget(const ReadFile data, const Person* const target);
double* maxDistancesFromTarget(const ReadFile data, const Person* const target);
void selectionSort(double* data, const size_t totalLen, const size_t useLen);
int compareDouble(const double* const p1, const double* const p2);
double squarekNN(const double* const distances, size_t dataLen, size_t k);
double roundkNN(const double* const distances, size_t dataLen, size_t k);
double weightedDistance(const Person* const person1, const Person* const person2);
double weightedMaxDistance(const Person* const person1, const Person* const person2);
ReadFile fileRead(const char* const fileName);
void fileWrite(const char* const fileName, const StatResult* target);
void freeset(void** p);

void main() {
	char ReadFileName[] = "데이터 세트/20232907-00.csv", writeFileName[] = "확률 분포 결과/20232907-00.csv";
	ReadFile datas[3][10] = { {0,} }, * pdatas = NULL;
	StatResult results[3][10] = { {0,} }, * presults;
	GaussianInfo standard;
	standard.mean.height = 182.9;
	standard.mean.weight = 67.7;
	standard.std[0][0] = 92;
	standard.std[1][1] = 72;
	standard.std[0][1] = standard.std[1][0] = 0;
	for (pdatas = *datas; pdatas < *datas + sizeof(datas) / sizeof(ReadFile); pdatas++) {
		ReadFileName[strlen(ReadFileName) - 6] = (char)((pdatas - *datas + 1) / 10) + '0';
		ReadFileName[strlen(ReadFileName) - 5] = (char)((pdatas - *datas + 1) % 10) + '0';
		*pdatas = fileRead(ReadFileName);
	}
	for (presults = *results, pdatas = *datas; presults < *results + sizeof(results) / sizeof(StatResult); presults++, pdatas++) {
		//printf("calculate:%d\n", (int)(presults - *results) + 1);
		*presults = getFunctionDistribution(*pdatas, standard);
	}

	for (pdatas = *datas; pdatas < *datas + sizeof(datas) / sizeof(ReadFile); pdatas++)
		free(pdatas->data);
	for (presults = *results; presults < *results + sizeof(results) / sizeof(StatResult); presults++) {
		writeFileName[strlen(writeFileName) - 6] = (char)((presults - *results + 1) / 10) + '0';
		writeFileName[strlen(writeFileName) - 5] = (char)((presults - *results + 1) % 10) + '0';
		fileWrite(writeFileName, presults);
	}
	for (presults = *results; presults < *results + sizeof(results) / sizeof(StatResult); presults++)
		free(presults->data);
}

//본격적인 연산
StatResult getFunctionDistribution(const ReadFile data, const GaussianInfo standard) {
	StatResult result;
	PersonChance* pdata;
	Person target, windowSize;
	double* distanceSet;
	GaussianInfo MLEresult = MLE(data);
	double std[2] = { sqrt(standard.std[0][0]) ,sqrt(standard.std[1][1]) };
	result.data = calloc(DATALEN, sizeof(PersonChance));
	if (result.data == NULL) {
		result.dataLen = 0;
		return result;
		exit(1);
	}
	pdata = result.data;

	for (int xdiff = -500, ydiff, midxFlag; xdiff <= 500; ) {
		midxFlag = xdiff % 100 != 0; //이때는 ydiff 범위 제한
		if (midxFlag) ydiff = -200;
		else ydiff = -500;
		while (ydiff <= 500) {
			if (pdata - result.data >= DATALEN)
				exit(1); //사전에 지정된 데이터 크기를 넘어가는 것은 오류로 보았음
			target.height = standard.mean.height + std[0] * xdiff / 100;
			target.weight = standard.mean.weight + std[1] * ydiff / 100;
			pdata->data = target;
			pdata->original = getGaussianValueatPoint(standard, target);
			pdata->MLEvalue = getGaussianValueatPoint(MLEresult, target);
			if (pdata->original == 0.0) pdata->MLEvalueError = 0.0;
			else pdata->MLEvalueError = (pdata->MLEvalue - pdata->original) / pdata->original;

			//파젠 창
			for (int i = 0; i < 3; i++) {
				windowSize.height = std[0] * (i + 1);
				windowSize.weight = std[1] * (i + 1);
				pdata->squareParzen[i] = squareParzenWindow(data, target, windowSize);
				if (pdata->squareParzen[i] == 0.0 && pdata->original == 0.0) pdata->squareParzenError[i] = 0.0;
				else pdata->squareParzenError[i] = (pdata->squareParzen[i] - pdata->original) / pdata->original;

				pdata->roundParzen[i] = roundParzenWindow(data, target, windowSize.height / 2 * heightWeight); //최댓값 사용
				if (pdata->original == 0.0) pdata->roundParzenError[i] = 0.0;
				else pdata->roundParzenError[i] = (pdata->roundParzen[i] - pdata->original) / pdata->original;
			}

			//k-NN
			distanceSet = distancesFromTarget(data, &target);
			selectionSort(distanceSet, data.dataLen, 7);
			for (int i = 0; i < 3; i++) {
				pdata->roundKNN[i] = roundkNN(distanceSet, data.dataLen, i * 2 + 3);
				if (pdata->original == 0.0) pdata->roundKNNError[i] = 0.0;
				else pdata->roundKNNError[i] = (pdata->roundKNN[i] - pdata->original) / pdata->original;
			}
			free(distanceSet);
			distanceSet = maxDistancesFromTarget(data, &target);
			selectionSort(distanceSet, data.dataLen, 7);
			for (int i = 0; i < 3; i++) {
				pdata->squareKNN[i] = squarekNN(distanceSet, data.dataLen, i * 2 + 3);
				if (pdata->original == 0.0) pdata->squareKNNError[i] = 0.0;
				else pdata->squareKNNError[i] = (pdata->squareKNN[i] - pdata->original) / pdata->original;
			}
			free(distanceSet);

			pdata++;
			if (xdiff >= -200 && xdiff <= 200 && ydiff >= -200 && ydiff < 200) ydiff += 10;
			//정확히 200일 때는 100이 올라야 하므로 마지막에 등호 제외
			else if (midxFlag && ydiff == 200) break;
			else ydiff += 100;
		}
		if (xdiff >= -200 && xdiff < 200) xdiff += 10;
		else xdiff += 100;
	}
	result.MLEresult = MLEresult;
	if (standard.mean.height == 0.0) result.MLEError.mean.height = 0.0;
	else result.MLEError.mean.height = (MLEresult.mean.height - standard.mean.height) / standard.mean.height;
	if (standard.mean.weight == 0.0) result.MLEError.mean.weight = 0.0;
	else result.MLEError.mean.weight = (MLEresult.mean.weight - standard.mean.weight) / standard.mean.weight;
	for (size_t i = 0; i < sizeof(result.MLEresult.std) / sizeof(result.MLEresult.std[0]); i++)
		for (size_t j = 0; j < sizeof(result.MLEresult.std[i]) / sizeof(result.MLEresult.std[i][0]); j++) {
			if (standard.std[i][j] == 0.0) result.MLEError.std[i][j] = 0.0;
			else result.MLEError.std[i][j] = (MLEresult.std[i][j] - standard.std[i][j]) / standard.std[i][j];
		}
	result.dataLen = pdata - result.data;
	return result;
}

//같은 조건에서의 데이터를 여러 번 통계냈을 때 이 평균값을 구함(보고서에 작성할 데이터 추출 용도로, 그래프 그릴 시에는 활용하지 않음)
//data 순서는 같아야 함
StatResult collectResult(const StatResult* const results, size_t len) {
	StatResult result;
	const StatResult* presults;
	PersonChance* pdata = calloc(results->dataLen, sizeof(PersonChance));
	double* values = malloc(len * sizeof(double)), * pvalues;
	if (pdata == NULL || values == NULL) {
		freeset(&pdata);
		freeset(&values);
		exit(1);
	}
	
	result.data = pdata;
	result.dataLen = results->dataLen;
	for (size_t i = 0; i < results->dataLen; i++) {
		pdata[i].data.height = results->data[i].data.height;
		pdata[i].data.weight = results->data[i].data.weight;

		for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
			*pvalues = presults->data[i].original;
		pdata[i].original = sumDouble(values, len) / len;
		for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
			*pvalues = presults->data[i].MLEvalue;
		pdata[i].MLEvalue = sumDouble(values, len) / len;
		for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
			*pvalues = presults->data[i].MLEvalueError;
		pdata[i].MLEvalueError = sumDouble(values, len) / len;
		for (int j = 0; j < 3; j++) {
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].squareParzen[j];
			pdata[i].squareParzen[j] = sumDouble(values, len) / len;
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].roundParzen[j];
			pdata[i].roundParzen[j] = sumDouble(values, len) / len;
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].squareKNN[j];
			pdata[i].squareKNN[j] = sumDouble(values, len) / len;
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].roundKNN[j];
			pdata[i].roundKNN[j] = sumDouble(values, len) / len;
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].squareParzenError[j];
			pdata[i].squareParzenError[j] = sumDouble(values, len) / len;
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].roundParzenError[j];
			pdata[i].roundParzenError[j] = sumDouble(values, len) / len;
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].squareKNNError[j];
			pdata[i].squareKNNError[j] = sumDouble(values, len) / len;
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->data[i].roundKNNError[j];
			pdata[i].roundKNNError[j] = sumDouble(values, len) / len;
		}
	}
	for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
		*pvalues = presults->MLEresult.mean.height;
	result.MLEresult.mean.height = sumDouble(values, len) / len;
	for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
		*pvalues = presults->MLEresult.mean.weight;
	result.MLEresult.mean.weight = sumDouble(values, len) / len;
	for (size_t i = 0; i < sizeof(result.MLEresult.std) / sizeof(result.MLEresult.std[0]); i++)
		for (size_t j = 0; j < sizeof(result.MLEresult.std[i]) / sizeof(result.MLEresult.std[i][0]); j++) {
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->MLEresult.std[i][j];
			result.MLEresult.std[i][j] = sumDouble(values, len) / len;
		}
	for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
		*pvalues = presults->MLEError.mean.height;
	result.MLEError.mean.height = sumDouble(values, len) / len;
	for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
		*pvalues = presults->MLEError.mean.weight;
	result.MLEError.mean.weight = sumDouble(values, len) / len;
	for (size_t i = 0; i < sizeof(result.MLEError.std) / sizeof(result.MLEError.std[0]); i++)
		for (size_t j = 0; j < sizeof(result.MLEError.std[i]) / sizeof(result.MLEError.std[i][0]); j++) {
			for (presults = results, pvalues = values; pvalues < values + len; presults++, pvalues++)
				*pvalues = presults->MLEError.std[i][j];
			result.MLEError.std[i][j] = sumDouble(values, len) / len;
		}
	free(values);
	return result;
}

//Gaussian MLE 연산 시행
GaussianInfo MLE(const ReadFile data) {
	GaussianInfo result = { 0, };
	Person* pdata;
	double* values = malloc(data.dataLen * sizeof(double)), * pvalues;
	double* values2 = malloc(data.dataLen * sizeof(double)), * pvalues2;
	double* matrixValue = malloc(data.dataLen * data.dataLen * sizeof(double)), * pmatrixValue;
	if (values == NULL || values2 == NULL || matrixValue == NULL) {
		freeset(&values);
		freeset(&values2);
		freeset(&matrixValue);
		exit(1);
	}
	for (pdata = data.data, pvalues = values; pdata < data.data + data.dataLen; pdata++, pvalues++)
		*pvalues = pdata->height;
	result.mean.height = sumDouble(values, data.dataLen) / data.dataLen;
	for (pdata = data.data, pvalues = values; pdata < data.data + data.dataLen; pdata++, pvalues++)
		*pvalues = pdata->weight;
	result.mean.weight = sumDouble(values, data.dataLen) / data.dataLen;
	//std
	for (pdata = data.data, pvalues = values; pdata < data.data + data.dataLen; pdata++, pvalues++)
		*pvalues = pdata->height - result.mean.height;
	for (pdata = data.data, pvalues2 = values2; pdata < data.data + data.dataLen; pdata++, pvalues2++)
		*pvalues2 = pdata->weight - result.mean.weight;
	for (pmatrixValue = matrixValue, pvalues = values; pvalues < values + data.dataLen; pvalues++)
		for (pvalues2 = values2; pvalues2 < values2 + data.dataLen; pvalues2++, pmatrixValue++)
			*pmatrixValue = *pvalues * *pvalues2;
	result.std[0][1] = sumDouble(matrixValue, data.dataLen * data.dataLen) / (data.dataLen * data.dataLen);
	result.std[1][0] = result.std[0][1];
	for (pvalues = values; pvalues < values + data.dataLen; pvalues++)
		*pvalues *= *pvalues; //제곱
	for (pvalues2 = values2; pvalues2 < values2 + data.dataLen; pvalues2++)
		*pvalues2 *= *pvalues2; //제곱
	result.std[0][0] = sumDouble(values, data.dataLen) / data.dataLen;
	result.std[1][1] = sumDouble(values2, data.dataLen) / data.dataLen;
	free(values);
	free(values2);
	free(matrixValue);
	return result;
}

//Gaussian 분포의 특정 지점에서 확률 분포 값 얻기
double getGaussianValueatPoint(const GaussianInfo gaussianInfo, const Person target) {
	double det = gaussianInfo.std[0][0] * gaussianInfo.std[1][1] - gaussianInfo.std[0][1] * gaussianInfo.std[1][0];
	Person diff;
	double diff_cov[2];
	diff.height = target.height - gaussianInfo.mean.height;
	diff.weight = target.weight - gaussianInfo.mean.weight;
	diff_cov[0] = diff.height * gaussianInfo.std[1][1] - diff.weight * gaussianInfo.std[1][0];
	diff_cov[0] /= det;
	diff_cov[1] = diff.weight * gaussianInfo.std[0][0] - diff.height * gaussianInfo.std[0][1];
	diff_cov[1] /= det;
	return exp((diff_cov[0] * diff.height + diff_cov[1] * diff.weight) * -1) / (2 * M_PI * sqrt(det));
}

//여러 double형을 더하는 함수(부동소수점의 precision 고려)
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

//사각형 파젠 창
double squareParzenWindow(const ReadFile data, const Person target, const Person windowSize) {
	size_t datasInWindow = 0;
	Person minimum, maximum;
	minimum.height = target.height - windowSize.height / 2;
	maximum.height = target.height + windowSize.height / 2;
	minimum.weight = target.weight - windowSize.weight / 2;
	maximum.weight = target.weight + windowSize.weight / 2;
	for (const Person* pdata = data.data; pdata < data.data + data.dataLen; pdata++)
		if (pdata->height >= minimum.height && pdata->height <= maximum.height &&
			pdata->weight >= minimum.weight && pdata->weight <= maximum.weight)
			datasInWindow++;

	return (double)datasInWindow / data.dataLen /
		(windowSize.height * heightWeight * windowSize.weight * weightWeight);
}

//원형 파젠 창
double roundParzenWindow(const ReadFile data, const Person target, double windowradius) {
	size_t datasInWindow = 0;
	windowradius *= windowradius; //거리계산 반환값이 제곱이기 때문
	for (const Person* pdata = data.data; pdata < data.data + data.dataLen; pdata++)
		if (weightedDistance(pdata, &target) <= windowradius)
			datasInWindow++;
	return (double)datasInWindow / data.dataLen / (windowradius * M_PI); //반지름 제곱은 이미 시행함
}

//주어진 점으로부터의 유클리드 거리의 제곱을 작은 순으로 정렬해 저장하는 배열
double* distancesFromTarget(const ReadFile data, const Person* const target) {
	const Person* pdata = data.data;
	double* result = malloc(data.dataLen * sizeof(double)), * presult;
	if (result == NULL) exit(1);
	presult = result;

	while (pdata < data.data + data.dataLen)
		*presult++ = weightedDistance(pdata++, target);

	return result;
}

//주어진 점으로부터의 최대 거리를 작은 순으로 정렬해 저장하는 배열
double* maxDistancesFromTarget(const ReadFile data, const Person* const target) {
	const Person* pdata = data.data;
	double* result = malloc(data.dataLen * sizeof(double)), * presult;
	if (result == NULL) exit(1);
	presult = result;

	while (pdata < data.data + data.dataLen)
		*presult++ = weightedMaxDistance(pdata++, target);

	return result;
}

//데이터가 가장 작은 몇 개만 찾아 선택정렬(전체 정렬 시 시간이 많이 소요되어 이를 처리하기 위한 것)
void selectionSort(double* data, const size_t totalLen, const size_t useLen) {
	for (double* p = data, *min, tmp; p < data + useLen; p++) {
		min = p;
		for (double* q = p + 1; q < data + totalLen; q++)
			if (*q < *min) min = q;
		tmp = *min;
		*min = *p;
		*p = tmp;
	}
}

int compareDouble(const double* const p1, const double* const p2) {
	if (*p1 > *p2) return 1;
	else if (*p1 < *p2) return -1;
	else return 0;
}

//사각형 kNN, k를 바꿔가며 여러 번 진행하기 위해 거리 계산은 그 결과를 가져옴
double squarekNN(const double* const distances, size_t dataLen, size_t k) {
	double windowSize = distances[k - 1];
	return (double)k / dataLen / (windowSize * windowSize * 4);
}

//원형 kNN, k를 바꿔가며 여러 번 진행하기 위해 거리 계산은 그 결과를 가져옴
double roundkNN(const double* const distances, size_t dataLen, size_t k) {
	return (double)k / dataLen / (distances[k - 1] * M_PI);//distances는 이미 제곱되어 있음
}

//유클리드 거리의 제곱을 반환(단순 거리 계산이 목적이면 별도로 제곱근 사용 필요)
double weightedDistance(const Person* const person1, const Person* const person2) {
	double height = person1->height - person2->height, weight = person1->weight - person2->weight;
	return (height * height) * (heightWeight * heightWeight) + (weight * weight) * (weightWeight * weightWeight);
}

//최대 거리 반환(사각형 kNN에 사용)
double weightedMaxDistance(const Person* const person1, const Person* const person2) {
	double height = person1->height - person2->height, weight = person1->weight - person2->weight;
	height = (height * height) * (heightWeight * heightWeight);
	weight = (weight * weight) * (weightWeight * weightWeight);
	if (height > weight) return height;
	else return weight;
}

//파일 읽기
ReadFile fileRead(const char* const fileName) {
	ReadFile result = { 0, };
	Person* data = NULL, * pdata = NULL;
	char readLine[LINELEN], * pReturn;
	FILE* file;
	size_t dataLen = 0;

	static const char tokDelimeter[] = ",\n";
#define get1stDelimeter tokDelimeter[0]
#define get2ndData(line) (strtok(strchr((line), get1stDelimeter), tokDelimeter)) //맨 앞 데이터 자름
	
	file = fopen(fileName, "rt");
	if (file == NULL) {
		printf("Cannot read file %s\n", fileName);
		exit(1);
	}
	fgets(readLine, sizeof(readLine) / sizeof(readLine[0]), file);
	dataLen = atoi(get2ndData(readLine)); //파일 형식: z,dataLen,2
	data = malloc(dataLen * sizeof(Person)); //dataLen==0이면 data=NULL
	if (data == NULL) {
		printf("Incorrect file %s(cannot get dataLen) or not enough memory\n", fileName);
		fclose(file);
		exit(1);
	}
	fgets(readLine, sizeof(readLine) / sizeof(readLine[0]), file); //index 설명 읽어 버리기
	pdata = data;
	while (!feof(file)) {
		pReturn = fgets(readLine, sizeof(readLine) / sizeof(readLine[0]), file); //파일 형식: index,height,weight
		if (pReturn == NULL || *readLine == '\0' || *readLine == '\n') break; //개행문자 하나만 남아 있어 파일을 읽게 되는 때 방지
		if ((size_t)(pdata - data) >= dataLen) { //재할당 필요 또는 예외 상황으로 처리도 가능
			fclose(file);
			free(data);
			exit(1);
		}
		pdata->height = atof(get2ndData(readLine));
		pdata++->weight = atof(strtok(NULL, tokDelimeter));
	}
	fclose(file);
	result.data = data;
	result.dataLen = pdata - data;
	return result;
}

//파일 쓰기
void fileWrite(const char* const fileName, const StatResult* target) {
	FILE* file = fopen(fileName, "wtx");
	if (file == NULL) {
		printf("Cannot write at %s\n", fileName);
		return; //파일에 못 쓴다고 못하는 일은 없음
	}
	//첫줄(index) (길어서 4번에 나눔)
	fputs("height,weight,original,MLEValue,squareParzen(1 std),squareParzen(2 std),squareParzen(3 std),roundParzen(1 std),roundParzen(2 std),", file);
	fputs("roundParzen(3 std),squareKNN(k=3),squareKNN(k=5),squareKNN(k=7),roundKNN(k=3),roundKNN(k=5),roundKNN(k=7),MLEvalueError,squareParzenError(1 std),", file);
	fputs("squareParzenError(2 std),squareParzenError(3 std),roundParzenError(1 std),roundParzenError(2 std),roundParzenError(3 std)", file);
	fputs(",squareKNNError(k=3),squareKNNError(k=5),squareKNNError(k=7),roundKNNError(k=3),roundKNNError(k=5),roundKNNError(k=7)\n", file);
	for (const PersonChance* pdata = target->data; pdata < target->data + target->dataLen; pdata++) {
		fprintf(file, "%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf",
			pdata->data.height, pdata->data.weight, pdata->original, pdata->MLEvalue,
			pdata->squareParzen[0], pdata->squareParzen[1], pdata->squareParzen[2],
			pdata->roundParzen[0], pdata->roundParzen[1], pdata->roundParzen[2],
			pdata->squareKNN[0], pdata->squareKNN[1], pdata->squareKNN[2],
			pdata->roundKNN[0], pdata->roundKNN[1], pdata->roundKNN[2]);
		fprintf(file, ",%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf\n",
			pdata->MLEvalueError,
			pdata->squareParzenError[0], pdata->squareParzenError[1], pdata->squareParzenError[2],
			pdata->roundParzenError[0], pdata->roundParzenError[1], pdata->roundParzenError[2],
			pdata->squareKNNError[0], pdata->squareKNNError[1], pdata->squareKNNError[2],
			pdata->roundKNNError[0], pdata->roundKNNError[1], pdata->roundKNNError[2]);
	}
	if (target->MLEresult.mean.height != 0 || target->MLEresult.std[0][0] != 0) {
		fprintf(file, "\nmean,std,,meanError,stdError\n");
		fprintf(file,"%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf\n%.45lf,%.45lf,%.45lf,%.45lf,%.45lf,%.45lf",
			target->MLEresult.mean.height, target->MLEresult.std[0][0], target->MLEresult.std[0][1],
			target->MLEError.mean.height, target->MLEError.std[0][0], target->MLEError.std[0][1],
			target->MLEresult.mean.weight, target->MLEresult.std[1][0], target->MLEresult.std[1][1],
			target->MLEError.mean.weight, target->MLEError.std[1][0], target->MLEError.std[1][1]);
	}
	fclose(file);
}

void freeset(void** p) {
	if (*p != NULL) {
		free(*p);
		*p = NULL;
	}
}