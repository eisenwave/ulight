// Java Syntax Highlighting Stress Test

package highlight.test;

import java.util.List;
import java.util.Map;
import static java.lang.Math.*;

/**
 * A comprehensive test of Java syntax highlighting.
 * This Javadoc comment tests documentation comments.
 * @param <T> a type parameter
 * @see java.util.List
 */
public class Java<T extends Comparable<T>> {

    // Primitive declarations and literals
    private boolean flag = true;
    protected byte b = 127;
    short s = 32767;
    int i = 2147483647;
    long l = 9223372036854775807L;
    float f = 3.14f;
    double d = 2.718281828;
    char c = 'A';
    char escaped = '\n';

    // String and text block literals
    String empty = "";
    String hello = "Hello, World!";
    String escaped_str = "Tab:\t Newline:\n Quote:\" Backslash:\\";
    String unicode_escape = "\u0048\u0065\u006c\u006c\u006f";
    String octal_escape = "\101\102\103";
    String text_block = """
            This is a text block.
            It spans multiple lines.
            "Quotes" are fine.
            """;

    // Numeric literals
    int dec = 1234567890;
    int dec_underscore = 1_000_000;
    int hex = 0xff;
    int hex_underscore = 0xDead_Cafe;
    int octal = 0777;
    int octal_underscore = 01_234;
    int binary = 0b1010;
    int binary_underscore = 0b1010_0101;
    long long_val = 100L;
    long hex_long = 0xFFFF_FFFF_FFFF_FFFFL;
    float float_val = 0.5f;
    float float_exp = 1.5e-10f;
    double double_val = 0.5d;
    double double_exp = 1.5e10;
    double hex_double = 0x1.fffffeP+127f;

    // Null literal
    Object null_val = null;

    // Array types
    int[] int_array = new int[10];
    String[][] matrix = new String[5][5];

    // Annotations
    @Override
    @SuppressWarnings("unchecked")
    @Deprecated(since = "1.5")
    public String toString() {
        return "Java";
    }

    // Method with modifiers
    public static final synchronized native void nativeMethod();

    // Control flow
    public int controlDemo(int x) {
        if (x < 0) {
            return -1;
        } else if (x == 0) {
            return 0;
        } else {
            return 1;
        }
    }

    // Switch statement and expression
    public String switchDemo(int day) {
        String result = switch (day) {
            case 1 -> "Monday";
            case 2 -> "Tuesday";
            case 3, 4, 5 -> "Weekday";
            default -> "Weekend";
        };

        switch (day) {
            case 1:
                result = "Mon";
                break;
            case 2:
                result = "Tue";
                break;
            default:
                result = "Other";
                break;
        }
        return result;
    }

    // Loops
    public void loopDemo() {
        for (int i = 0; i < 10; i++) {
            System.out.println(i);
        }
        int j = 0;
        while (j < 5) {
            j++;
        }
        do {
            j--;
        } while (j > 0);
        for (String s : List.of("a", "b", "c")) {
            System.out.println(s);
        }
    }

    // Exception handling
    public void exceptionDemo() throws Exception {
        try {
            throw new RuntimeException("Oops");
        } catch (RuntimeException e) {
            System.err.println(e.getMessage());
        } catch (Exception e) {
            throw e;
        } finally {
            System.out.println("Finally");
        }
    }

    // Generics and type parameters
    public <U> List<U> genericMethod(U value) {
        Map<String, ? extends List<? super U>> map = null;
        return List.of(value);
    }

    // Lambda expressions
    public void lambdaDemo() {
        Runnable r = () -> System.out.println("Hello");
        java.util.function.Function<Integer, String> f = (x) -> String.valueOf(x);
        java.util.function.BiFunction<Integer, Integer, Integer> add = (a, b) -> a + b;
    }

    // Method references
    public void methodRefDemo() {
        java.util.function.Supplier<String> sup = String::new;
        java.util.function.Function<String, Integer> len = String::length;
    }

    // instanceof pattern matching
    public String instanceofDemo(Object obj) {
        if (obj instanceof String str && str.length() > 0) {
            return str;
        }
        if (obj instanceof Integer num) {
            return num.toString();
        }
        return "unknown";
    }

    // Operators
    public int operatorsDemo() {
        int x = 10;
        x += 5;
        x -= 3;
        x *= 2;
        x /= 4;
        x %= 3;
        x &= 0xff;
        x |= 0x100;
        x ^= 0x0f;
        x <<= 2;
        x >>= 1;
        x >>>= 1;
        return ~x & 0xff;
    }

    // Ternary operator
    public String ternaryDemo(boolean cond) {
        return cond ? "yes" : "no";
    }

    // this and super
    public Java() {
        super();
        this.i = 0;
    }

    // Nested class
    static class Nested {
        int value;
    }

    // Inner class
    class Inner {
        int getOuterI() {
            return i;
        }
    }
}

// Interface
interface Clickable {
    void click();
    default void doubleClick() {
        System.out.println("Double click");
    }
}

// Enum
enum Direction {
    NORTH, SOUTH, EAST, WEST;

    public Direction opposite() {
        return switch (this) {
            case NORTH -> SOUTH;
            case SOUTH -> NORTH;
            case EAST -> WEST;
            case WEST -> EAST;
        };
    }
}

// Record
record Point(int x, int y) {
    public static final Point ORIGIN = new Point(0, 0);
}

// Sealed class hierarchy
sealed interface Shape permits Circle, Rectangle, Triangle {
}

record Circle(double radius) implements Shape {
}

record Rectangle(double width, double height) implements Shape {
}

final class Triangle implements Shape {
    final double base;
    final double height;

    Triangle(double base, double height) {
        this.base = base;
        this.height = height;
    }
}
