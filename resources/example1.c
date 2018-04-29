int main() {
    int x, a;
    int b = 5;
    if (a > 0)
        x = 3 + b;
    else
        x = 3 - b;

    if (a > 10)
        x = 3 + x;
    else
        x = 3 - x;
    return x;
}
