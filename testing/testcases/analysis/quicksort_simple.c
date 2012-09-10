void swap(int *a, int *b)
{
	int t=*a; *a=*b; *b=t;
}

/*
 * Quicksort simple version as given at
 * http://alienryderflex.com/quicksort/
 */
void sort(int arr[], int beg, int end)
{
  if (end > beg + 1)
  {
    int piv = arr[beg], l = beg + 1, r = end;
    while (l < r)
    {
      if (arr[l] <= piv)
        l++;
      else
        swap(&arr[l], &arr[--r]);
    }
    swap(&arr[--l], &arr[beg]);
    sort(arr, beg, l);
    sort(arr, r, end);
  }
}

int main()
{
#define NUM 100
	int arrry[NUM];
	int i;
	for (i = NUM; i > 0; --i) {
		arrry[NUM-i] = i;
	}
	sort(arrry, 0, NUM-1);
	return 0;
}
