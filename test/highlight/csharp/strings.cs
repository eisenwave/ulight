// C# String Literals Test

class StringTest
{
    // Regular string literals
    string s1 = "";
    string s2 = "Hello";
    string s3 = "Hello, World!";
    string s4 = "Tab:\t";
    string s5 = "Newline:\n";
    string s6 = "Quote:\"";
    string s7 = "Backslash:\\";
    string s8 = "Bell:\a";
    string s9 = "Backspace:\b";
    string s10 = "Form feed:\f";
    string s11 = "Carriage return:\r";
    string s12 = "Vertical tab:\v";
    string s13 = "Null:\0";

    // Unicode escape sequences
    string u1 = "\u0048";
    string u2 = "\u0048\u0065\u006c\u006c\u006f";
    string u3 = "\U00000048";

    // Hex escape sequences
    string x1 = "\x41";
    string x2 = "\x0041";
    string x3 = "\x48\x65\x6c\x6c\x6f";
    string x4 = "\x12345";

    // UTF-8 string literals
    string utf8_1 = "Hello"u8;
    string utf8_2 = "Hello"U8;

    // Character literals
    char c1 = 'A';
    char c2 = '\n';
    char c3 = '\t';
    char c4 = '\'';
    char c5 = '\\';
    char c6 = '\u0048';
    char c7 = '\x41';
    char c8 = 'é';

    // Verbatim string literals
    string v1 = @"";
    string v2 = @"Hello";
    string v3 = @"C:\Users\name\file.txt";
    string v4 = @"Line 1
Line 2";
    string v5 = @"Quote: """"";
    string v6 = @"path\to\file";

    // Verbatim with UTF-8 suffix
    string vu1 = @"Hello"u8;
    string vu2 = @"Hello"U8;

    // Raw string literals
    string r1 = """Single line raw string""";
    string r2 = """
        Multi-line
        raw string
        """;
    string r3 = """"
        Raw string with four quotes
        """";
    string r4 = """raw"""u8;

    // Interpolated regular strings
    string i1 = $"Hello, {name}!";
    string i2 = $"Value: {x + y}";
    string i3 = $"Nested: {obj.ToString()}";
    string i4 = $"Escaped braces: {{not an expression}}";
    string i5 = $"Format: {value:X}";
    string i6 = $"Alignment: {value,10}";
    string i7 = $"Format+Align: {value,10:F2}";

    // Interpolated with UTF-8 suffix
    string iu1 = $"Hello"u8;

    // Interpolated verbatim strings
    string iv1 = $@"Path: {path}\file";
    string iv2 = @$"Path: {path}\file";
    string iv3 = $@"Line 1
Line 2: {value}";
    string iv4 = @$"Line 1
Line 2: {value}";
    string iv5 = $@"Quote: """" and brace: {{}}";
    string ir1 = $"""Value: {x}""";
    string ir2 = $$"""Value: {{x}}""";
    string in1 = $"{M("x")}";

    // Preprocessor directives
    #define DEBUG
    #undef TRACE
    #if DEBUG
    #elif RELEASE
    #else
    #endif
    #region MyRegion
    #endregion
    #error Error message
    #warning Warning message
    #line 42 "file.cs"
    #nullable enable
    #nullable disable
    #pragma warning disable CS0618
}
