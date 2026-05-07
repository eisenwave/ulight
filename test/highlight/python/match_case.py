def classify(value):
    match value:
        case 0:
            return "zero"
        case int() as n if n > 0:
            return "positive int"
        case _:
            return "other"
