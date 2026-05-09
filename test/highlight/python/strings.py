"\n."
"\123."
"\xff."
"\u1234."
"\U12345678."

r"\n."
r"\"."
r"\U12345678."
R"\U12345678."
rb"\u1234."
b"\u1234."

"\N{LATIN CAPITAL LETTER A}."
b"\N{LATIN CAPITAL LETTER A}."

# These are deprecated in Python 3.12 but not yet removed:
"\400."
b"\777."
