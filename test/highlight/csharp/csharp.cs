// C# Syntax Highlighting Stress Test

using System;
using System.Collections.Generic;
using System.Linq;
using static System.Math;

/// <summary>
/// A comprehensive test of C# syntax highlighting.
/// This XML doc comment tests documentation comments.
/// </summary>
/// <typeparam name="T">A type parameter</typeparam>
/// <seealso cref="List{T}"/>
public class CSharp<T> where T : IComparable<T>
{
    // Primitive declarations and literals
    private bool flag = true;
    protected byte b = 255;
    short s = 32767;
    int i = 2147483647;
    long l = 9223372036854775807L;
    float f = 3.14f;
    double d = 2.718281828;
    decimal m = 1.2345m;
    char c = 'A';
    char escaped = '\n';
    char hex_escaped = '\x41';
    char unicode_escaped = '\u0048';
    string str = "Hello, World!";
    object obj = null;

    // Unsigned integer types
    byte ub = 255;
    sbyte sb = -128;
    ushort us = 65535;
    uint ui = 4294967295u;
    ulong ul = 18446744073709551615UL;
    nint ni = 42;
    nuint nu = 42;

    // String literals
    string empty = "";
    string hello = "Hello, World!";
    string escaped_str = "Tab:\t Newline:\n Quote:\" Backslash:\\";
    string unicode_escape = "\u0048\u0065\u006c\u006c\u006f";
    string hex_escape = "\x48\x65\x6c\x6c\x6f";

    // Verbatim strings
    string verbatim = @"C:\Users\name\file.txt";
    string verbatim_multiline = @"Line 1
Line 2
""Quoted"" text";

    // Raw string literals
    string raw_single = """This is a raw string""";
    string raw_multi = """
        This is a multi-line
        raw string literal.
        "Quotes" are fine.
        """;

    // Interpolated strings
    string interpolated = $"Hello, {name}!";
    string interpolated_verbatim = $@"Path: {path}\file";
    string interpolated_at_dollar = @$"Path: {path}\file";
    string interpolated_escaped = $"{{Not an expression}}";

    // UTF-8 string literals
    string utf8_str = "Hello"u8;
    string utf8_verbatim = @"Hello"u8;

    // Numeric literals
    int dec = 1234567890;
    int dec_underscore = 1_000_000;
    int hex = 0xff;
    int hex_underscore = 0xDead_Cafe;
    int binary = 0b1010;
    int binary_underscore = 0b1010_0101;
    long long_val = 100L;
    long hex_long = 0xFFFF_FFFF_FFFF_FFFFL;
    uint uint_val = 100u;
    ulong ulong_val = 100UL;
    float float_val = 0.5f;
    float float_exp = 1.5e-10f;
    double double_val = 0.5d;
    double double_exp = 1.5e10;
    double hex_double = 0x1.fffffeP+127d;
    decimal decimal_val = 1.2345m;

    // Array types
    int[] int_array = new int[10];
    string[][] matrix = new string[5][];

    // Nullable value types
    int? nullable_int = null;

    // Null coalescing
    string coalesce = nullable_str ?? "default";
    nullable_str ??= "assigned";

    // Range operator
    var range = 1..10;
    var range_from_end = ^1..^0;

    // Lambda expressions
    Func<int, int> lambda = x => x * 2;

    // Switch expression
    var result = x switch
    {
        1 => "one",
        2 => "two",
        _ => "other"
    };

    // Pattern matching
    if (obj is string s && s.Length > 0) { }
    if (obj is int or long or float) { }
    if (obj is not null) { }

    // Method with modifiers
    public static async Task<int> MethodAsync(int x) => await Task.FromResult(x);

    // Control flow
    public int ControlDemo(int x)
    {
        if (x < 0)
        {
            return -1;
        }
        else if (x == 0)
        {
            return 0;
        }
        else
        {
            for (int j = 0; j < x; j++)
            {
                Console.WriteLine(j);
            }

            while (x > 10)
            {
                x /= 2;
            }

            do
            {
                x--;
            } while (x > 0);

            foreach (var item in items)
            {
                Process(item);
            }
        }

        switch (x)
        {
            case 1:
                goto case 2;
            case 2:
                return 42;
            default:
                break;
        }

        try
        {
            DangerousOperation();
        }
        catch (Exception ex) when (ex.Message != null)
        {
            Console.WriteLine(ex.Message);
        }
        finally
        {
            Cleanup();
        }

        return 0;
    }

    // LINQ query expression
    public IEnumerable<int> QueryDemo(List<int> numbers)
    {
        return from n in numbers
               where n > 0
               orderby n descending
               select n;
    }

    // Properties
    public string Name { get; set; }
    public string ReadOnlyName { get; init; }
    public int Computed => 42;

    // Events
    public event EventHandler Changed;

    // Indexer
    public string this[int index]
    {
        get => _items[index];
        set => _items[index] = value;
    }

    // Operator overloading
    public static CSharp<T> operator +(CSharp<T> a, CSharp<T> b) => a;

    // Deconstructor
    public void Deconstruct(out int x, out int y) { x = 1; y = 2; }
    var (a, b) = point;

    // Record declaration
    public record Point(int X, int Y);

    // Attributes
    [Obsolete("Use NewMethod instead")]
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void OldMethod() { }

    // Unsafe code
    unsafe void UnsafeMethod()
    {
        int x = 42;
        int* ptr = &x;
        *ptr = 100;
    }

    // Using declaration
    using var file = new StreamReader("test.txt");

    // Lock statement
    lock (_lockObject)
    {
        _counter++;
    }

    // With expression
    var newPoint = point with { X = 10 };

    // Target-typed new
    CSharp<T> instance = new();

    // Nameof expression
    string name = nameof(MethodAsync);

    // Sizeof expression
    int size = sizeof(int);

    // Typeof expression
    Type type = typeof(CSharp<T>);

    // Default values
    int defaultInt = default;
    T defaultT = default(T);

    // Stackalloc
    Span<int> span = stackalloc int[10];

    // Checked/unchecked
    checked
    {
        int result = x + y;
    }

    // Is expression
    if (obj is string) { }

    // As expression
    var str2 = obj as string;

    // Ref local
    ref int refLocal = ref array[0];

    // Discard
    _ = ComputeSomething();

    // Function pointer (unsafe)
    delegate*<int, int, int> funcPtr = &Add;

    // Escaped identifiers
    var @class = "escaped";
    var @for = "also escaped";
}
