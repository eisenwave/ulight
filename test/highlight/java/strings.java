// Java String, Character, and Text Block Literals Test

class Strings {
    // Character literals
    char c1 = 'a';
    char c2 = 'Z';
    char c3 = '0';
    char c4 = '%';
    char c5 = '\t';
    char c6 = '\n';
    char c7 = '\\';
    char c8 = '\'';
    char c9 = '\u03a9';
    char c10 = '\uFFFF';
    char c11 = '\177';

    // String literals
    String s1 = "";
    String s2 = "Hello";
    String s3 = "Hello, World!";
    String s4 = "Tab:\t here";
    String s5 = "Newline:\n here";
    String s6 = "Quote:\" here";
    String s7 = "Backslash:\\ here";
    String s8 = "Single quote:\' here";
    String s9 = "Form feed:\f here";
    String s10 = "Carriage return:\r here";
    String s11 = "\"";
    String s12 = "\\";
    String s13 = "\u0048\u0065\u006c\u006c\u006f";
    String s14 = "\101\102\103";

    // Text blocks
    String tb1 = """
            """;

    String tb2 = """
            Hello, World!
            """;

    String tb3 = """
            This is a text block.
            It can span multiple lines.
            """;

    String tb4 = """
            "Quotes" are fine in text blocks.
            """;

    String tb5 = """
            Text blocks can contain \n escape sequences too.
            """;

    String tb6 = """
            Backslashes like \\ are also escapes.
            """;

    String tb7 = """
            Three quotes: \"""
            """;

    String tb8 = """
            <html>
                <body>
                    <p>Hello, world</p>
                </body>
            </html>
            """;
}
