// Java Number Literals Test

class Numbers {
    // Decimal integers
    int d0 = 0;
    int d1 = 1;
    int d2 = 1234567890;
    int d3 = 1_000_000;
    int d4 = 2_147_483_647;

    // Long suffix
    long l1 = 0L;
    long l2 = 123L;
    long l3 = 1_000_000L;
    long l4 = 9_223_372_036_854_775_807L;

    // Hexadecimal
    int h1 = 0x0;
    int h2 = 0xff;
    int h3 = 0xFF;
    int h4 = 0xDead_Cafe;
    int h5 = 0x7fff_ffff;
    long h6 = 0xFFFF_FFFF_FFFF_FFFFL;

    // Binary
    int b1 = 0b0;
    int b2 = 0b1;
    int b3 = 0b1010;
    int b4 = 0b1010_0101;
    int b5 = 0b1111_1111_1111_1111_1111_1111_1111_1111;

    // Octal
    int o1 = 00;
    int o2 = 0777;
    int o3 = 01_234;
    int o4 = 0177_7777_7777;

    // Floating-point decimal
    float f1 = 0f;
    float f2 = 0.5f;
    float f3 = .5f;
    float f4 = 1.0f;
    float f5 = 3.14f;
    float f6 = 1e1f;
    float f7 = 2.f;
    float f8 = .3f;
    float f9 = 6.022137e+23f;
    float f10 = 1e-9f;

    // Floating-point double
    double d5 = 0.0;
    double d6 = 0.5d;
    double d7 = .5;
    double d8 = 1.0d;
    double d9 = 3.14;
    double d10 = 1e1;
    double d11 = 2.;
    double d12 = .3;
    double d13 = 1e-9d;
    double d14 = 1e137;

    // Hexadecimal floating-point
    double hf1 = 0x1.fffffeP+127f;
    double hf2 = 0x0.000002P-126f;
    double hf3 = 0x1.0P-149f;
    double hf4 = 0x1.f_ffff_ffff_ffffP+1023;
}
