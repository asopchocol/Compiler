#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int solution(int arr[], size_t arr_len);

int main(void) {
	int a[] = { 2, 6, 8, 14 };

	solution(a, 4);

	return 0;
}

int solution(int arr[], size_t arr_len) {
	int answer = 0;
	int** pArr;
	int j = 2;
	int k;

	pArr = (int**)malloc(sizeof(int*) * arr_len);

	for (int i = 0; i < arr_len; i++) {
		pArr[i] = (int*)malloc(sizeof(int) * 100);
	}

	for (int i = 0; i < arr_len; i++) {  // 배열의 개수만큼 반복
		k = 0;
		for (j; j <= arr[i]; j++) {		 // 배열의 값을 소인수분해하여 새로운 배열에 저장
			while (arr[i] % j == 0) {	 // 값이 j값으로 나누어지면
				arr[i] /= j;			 // j로 나눈값을 저장
				pArr[i][k] = j;				 // pArr값을
				k++;
			}
		}
	}


	free(pArr);

	return answer;
}