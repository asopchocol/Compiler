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

	for (int i = 0; i < arr_len; i++) {  // �迭�� ������ŭ �ݺ�
		k = 0;
		for (j; j <= arr[i]; j++) {		 // �迭�� ���� ���μ������Ͽ� ���ο� �迭�� ����
			while (arr[i] % j == 0) {	 // ���� j������ ����������
				arr[i] /= j;			 // j�� �������� ����
				pArr[i][k] = j;				 // pArr����
				k++;
			}
		}
	}


	free(pArr);

	return answer;
}