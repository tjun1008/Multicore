#include <omp.h>
#include <iostream>

using namespace std;

//open mp 활성화 시키기

int main()
{
	
	int sum = 0;

#pragma omp parallel
	{
		int num_thread = omp_get_num_threads();
		
		for (int i = 0; i < 5000'0000/num_thread; ++i)
#pragma omp critical
			sum += 2;
	}
			cout << "Sum = " << sum << endl;
	
}