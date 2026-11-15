int main(void) {
    int n = 0;
    int sum = 0;
    while (n != 5) {
        sum = sum + n;
        n = n + 1;
    }
    if (sum == 10) { sum = sum + 5; } else { sum = 0; }
    if (sum != 15) { sum = 0; } else { sum = sum + 1; }
    return sum;
}
