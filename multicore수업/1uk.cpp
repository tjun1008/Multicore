#include <iostream>
using namespace std;

int sum;

int main()
{
	for (auto i = 0; i < 5000'0000; ++i)
		sum += 2;

	cout << "sum = " << sum << endl;
}