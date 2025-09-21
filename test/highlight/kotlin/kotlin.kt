// Kotlin Syntax Highlighting Stress Test

package highlight.test

import kotlin.math.*
import kotlinx.coroutines.*

// Top-level properties and constants
const val CONST_VAL = 42
val lazyVal: String by lazy { "lazy" }
var mutableVar: Int = 10

// Classes, objects, interfaces
data class Person(val id: Int, var name: String)

class Box<T>(val value: T) {
    fun get(): T = value
}

interface Clickable {
    fun click()
}

open class Base(open val x: Int) {
    open fun foo(): String = "Base: $x"
}

class Derived(override val x: Int) : Base(x), Clickable {
    override fun foo(): String = "Derived: $x"
    override fun click() { println("Clicked!") }
}

object Singleton {
    val data = listOf(1, 2, 3)
}

// Enum and sealed classes
enum class Direction { NORTH, SOUTH, EAST, WEST }

sealed class Expr
class Const(val number: Double) : Expr()
class Sum(val left: Expr, val right: Expr) : Expr()
object NotANumber : Expr()

// Functions with many parameter styles
fun fancy(
    a: Int,
    b: String = "default",
    vararg numbers: Int,
    c: (Int) -> Unit
): String {
    numbers.forEach { c(it) }
    return "$a:$b"
}

inline fun <reified T> genericFun(value: T): String = T::class.simpleName ?: "Unknown"

// Extensions
fun String.shout(): String = uppercase()

// Lambdas and higher-order functions
val add: (Int, Int) -> Int = { x, y -> x + y }

// Control flow
fun controlDemo(x: Int): String = when {
    x < 0 -> "negative"
    x == 0 -> "zero"
    x in 1..10 -> "small"
    else -> "big"
}

// Loops
fun loops() {
    for (i in 0 until 10 step 2) println(i)
    var j = 0
    while (j < 3) {
        println(j)
        j++
    }
    do {
        println("once")
    } while (false)
}

// Null safety and Elvis operator
fun safeLen(s: String?): Int = s?.length ?: -1

// Smart casts
fun describe(obj: Any): String = when (obj) {
    is Int -> "Int: $obj"
    is String -> "String of length ${obj.length}"
    else -> "Unknown"
}

// Coroutines
suspend fun suspendFun(): Int {
    delay(10)
    return 123
}

fun coroutineDemo() = runBlocking {
    val d = async { suspendFun() }
    println("Result: ${d.await()}")
}

// Generics, variance, typealiases
typealias StringMap = Map<String, String>

class Source<out T>(private val t: T) {
    fun produce(): T = t
}

class Sink<in T> {
    fun consume(t: T) { println(t) }
}

// Operator overloading
data class Vector(val x: Int, val y: Int) {
    operator fun plus(other: Vector) = Vector(x + other.x, y + other.y)
}

// Companion objects and annotations
class WithCompanion {
    companion object {
        const val MAGIC = 7
    }
}

@Target(AnnotationTarget.CLASS, AnnotationTarget.FUNCTION)
annotation class Fancy(val why: String)

@Fancy("demo")
class Annotated

// Exception handling
fun mayFail(x: Int) {
    try {
        require(x >= 0) { "x must be non-negative" }
    } catch (e: IllegalArgumentException) {
        println("Oops: ${e.message}")
    } finally {
        println("Finally block")
    }
}

// Main entry point
fun main() {
    println(CONST_VAL)
    println(Person(1, "Alice"))
    println(Derived(10).foo())
    println(Singleton.data)
    println(Direction.NORTH)
    println(controlDemo(5))
    loops()
    println(safeLen(null))
    println(describe("hi"))
    coroutineDemo()
    val v1 = Vector(1, 2)
    val v2 = Vector(3, 4)
    println(v1 + v2)
}
