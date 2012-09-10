void quicksort(int* data, int N)
{
  int i, j;
  int v, t;
 
  if( N <= 1 )
    return;
 
  // Partition elements
  v = data[0];
  i = 0;
  j = N;
  for(;;)
  {
    while(data[++i] < v && i < N) { }
    while(data[--j] > v) { }
    if( i >= j )
      break;
    t = data[i];
    data[i] = data[j];
    data[j] = t;
  }
  t = data[i-1];
  data[i-1] = data[0];
  data[0] = t;
  quicksort(data, i-1);
  quicksort(data+i, N-i);
}


int main()
{
#define NUM 100
	int arrry[NUM];
	int i;
	for (i = NUM; i > 0; --i) {
		arrry[NUM-i] = i;
	}
	quicksort(arrry, 100);

	return 0;
}
