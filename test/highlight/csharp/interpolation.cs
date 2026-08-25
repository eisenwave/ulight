// C# String Interpolation Highlighting Test

class InterpolationTest
{
    // Interpolated regular strings
    string i1 = $"Hello, {name}!";
    string i2 = $"Value: {x + y}";
    string i3 = $"Nested: {obj.ToString()}";
    string i4 = $"Method: {M("arg")}";

    // Escaped braces
    string e1 = $"{{not an expression}}";
    string e2 = $"Mixed: {{literal}} and {expr}";

    // Format and alignment
    string f1 = $"{value:X}";
    string f2 = $"{value,10}";
    string f3 = $"{value,10:F2}";

    // Interpolated verbatim strings
    string v1 = $@"Path: {path}\file";
    string v2 = @$"Path: {path}\file";
    string v3 = $@"Line 1
Line 2: {value}";
    string v4 = $@"Quote: """" and brace: {{}}";

    // Interpolated raw strings
    string r1 = $"""Value: {x}""";
    string r2 = $$"""Value: {{x}}""";
    string r3 = $$$"""Value: {{{x}}}""";
    string r4 = $$"""Only single braces are literal: {x}""";

    // Interpolated string with UTF-8 suffix
    string u1 = $"Hello"u8;
}
