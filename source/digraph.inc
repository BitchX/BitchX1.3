/*
 * diagraph.inc: included several times by translat.c
 */

DiLo(' ')	DiHi(0xa8)	DiDi(0x22)	/* Digital VT300 / VT400 */
DiLo(' ')	DiHi(0xb4)	DiDi(0x27)	/* Digital VT300 / VT400 */

/*
 * We don't really need to be *that* pedantic, so let's skip the useless
 * ones:  /Tomten
 */
/* DiLo(' ')	DiHi(' ')	DiDi(0xa0) */	/* Digital VT300 / VT400 */
/* DiLo(' ')	DiHi('^')	DiDi('^')  */	/* Digital VT300 / VT400 */
/* DiLo(' ')	DiHi('`')	DiDi('`')  */	/* Digital VT300 / VT400 */
/* DiLo(' ')	DiHi('~')	DiDi('~')  */	/* Digital VT300 / VT400 */
/* DiLo(' ')	DiHi(0xa8)	DiDi(0xa8) */	/* Digital VT300 / VT400 */

DiLo('!')	DiHi('!')	DiDi(0xa1)	/* Digital VT300 / VT400 */
DiLo('!')	DiHi('P')	DiDi(0xb6)	/* Digital VT300 / VT400 */
DiLo('!')	DiHi('S')	DiDi(0xa7)	/* Digital VT300 / VT400 */
DiLo('!')	DiHi('^')	DiDi(0xa6)	/* Digital VT300 / VT400 */
DiLo('!')	DiHi('~')	DiDi(0xa1)

DiLo(0x22)	DiHi(0x22)	DiDi(0xa8)	/* Digital VT300 / VT400 */
DiLo(0x22)	DiHi('>')	DiDi(0xab)
DiLo(0x22)	DiHi('<')	DiDi(0xbb)
DiLo(0x22)	DiHi('A')	DiDi(0xc4)
DiLo(0x22)	DiHi('E')	DiDi(0xcb)
DiLo(0x22)	DiHi('I')	DiDi(0xcf)
DiLo(0x22)	DiHi('O')	DiDi(0xd6)
DiLo(0x22)	DiHi('U')	DiDi(0xdc)
DiLo(0x22)	DiHi('a')	DiDi(0xe4)
DiLo(0x22)	DiHi('e')	DiDi(0xeb)
DiLo(0x22)	DiHi('i')	DiDi(0xef)
DiLo(0x22)	DiHi('o')	DiDi(0xf6)
DiLo(0x22)	DiHi('u')	DiDi(0xfc)
DiLo(0x22)	DiHi('y')	DiDi(0xff)

DiLo(0x27)	DiHi(0x27)	DiDi(0xb4)	/* Digital VT300 / VT400 */
DiLo(0x27)	DiHi('A')	DiDi(0xc1)
DiLo(0x27)	DiHi('E')	DiDi(0xc9)
DiLo(0x27)	DiHi('I')	DiDi(0xcd)
DiLo(0x27)	DiHi('O')	DiDi(0xd3)
DiLo(0x27)	DiHi('U')	DiDi(0xda)
DiLo(0x27)	DiHi('Y')	DiDi(0xdd)
DiLo(0x27)	DiHi('a')	DiDi(0xe1)
DiLo(0x27)	DiHi('e')	DiDi(0xe9)
DiLo(0x27)	DiHi('i')	DiDi(0xed)
DiLo(0x27)	DiHi('o')	DiDi(0xf3)
DiLo(0x27)	DiHi('u')	DiDi(0xfa)

DiLo('(')	DiHi('(')	DiDi('[')	/* Digital VT300 / VT400 */
DiLo('(')	DiHi('-')	DiDi('{')	/* Digital VT300 / VT400 */

DiLo(')')	DiHi(')')	DiDi(']')	/* Digital VT300 / VT400 */
DiLo(')')	DiHi('-')	DiDi('}')	/* Digital VT300 / VT400 */

DiLo('*')	DiHi('A')	DiDi(0xc5)	/* Digital VT300 / VT400 */
DiLo('*')	DiHi('a')	DiDi(0xe5)

DiLo('+')	DiHi('+')	DiDi('#')	/* Digital VT300 / VT400 */
DiLo('+')	DiHi('-')	DiDi(0xb1)	/* Digital VT300 / VT400 */

DiLo(',')	DiHi(',')	DiDi(0xb8)	/* Digital VT300 / VT400 */
DiLo(',')	DiHi('-')	DiDi(0xac)	/* Digital VT300 / VT400 */
DiLo(',')	DiHi('C')	DiDi(0xc7)
DiLo(',')	DiHi('c')	DiDi(0xe7)

DiLo('-')	DiHi('-')	DiDi(0xad)	/* Digital VT300 / VT400 */
DiLo('-')	DiHi(':')	DiDi(0xf7)	/* Digital VT300 / VT400 */
DiLo('-')	DiHi('D')	DiDi(0xd0)
DiLo('-')	DiHi('L')	DiDi(0xa3)	/* Digital VT300 / VT400 */
DiLo('-')	DiHi('Y')	DiDi(0xa5)	/* Digital VT300 / VT400 */
DiLo('-')	DiHi('^')	DiDi(0xaf)	/* Digital VT300 / VT400 */
DiLo('-')	DiHi('a')	DiDi(0xaa)
DiLo('-')	DiHi('d')	DiDi(0xf0)
DiLo('-')	DiHi('o')	DiDi(0xba)

DiLo('.')	DiHi('^')	DiDi(0xb7)	/* Digital VT300 / VT400 */

DiLo('/')	DiHi('/')	DiDi('\\')	/* Digital VT300 / VT400 */
DiLo('/')	DiHi('<')	DiDi('\\')	/* Digital VT300 / VT400 */
DiLo('/')	DiHi('C')	DiDi(0xa2)	/* Digital VT300 / VT400 */
DiLo('/')	DiHi('O')	DiDi(0xd8)
DiLo('/')	DiHi('U')	DiDi(0xb5)	/* Digital VT300 / VT400 */
DiLo('/')	DiHi('^')	DiDi('|')	/* Digital VT300 / VT400 */
DiLo('/')	DiHi('o')	DiDi(0xf8)

DiLo('0')	DiHi('C')	DiDi(0xa9)	/* Digital VT300 / VT400 */
DiLo('0')	DiHi('S')	DiDi(0xa7)	/* Digital VT300 / VT400 */
DiLo('0')	DiHi('X')	DiDi(0xa4)	/* Digital VT300 / VT400 */
DiLo('0')	DiHi('^')	DiDi(0xb0)	/* Digital VT300 / VT400 */

DiLo('1')	DiHi('^')	DiDi(0xb9)	/* Digital VT300 / VT400 */
DiLo('1')	DiHi('2')	DiDi(0xbd)	/* Digital VT300 / VT400 */
DiLo('1')	DiHi('4')	DiDi(0xbc)	/* Digital VT300 / VT400 */

DiLo('2')	DiHi('^')	DiDi(0xb2)	/* Digital VT300 / VT400 */

DiLo('3')	DiHi('4')	DiDi(0xbe)	/* Digital VT300 / VT400 */
DiLo('3')	DiHi('^')	DiDi(0xb3)	/* Digital VT300 / VT400 */

DiLo('<')	DiHi('<')	DiDi(0xab)	/* Digital VT300 / VT400 */

DiLo('=')	DiHi('L')	DiDi(0xa3)	/* Digital VT300 / VT400 */
DiLo('=')	DiHi('Y')	DiDi(0xa5)	/* Digital VT300 / VT400 */

DiLo('>')	DiHi('>')	DiDi(0xbb)	/* Digital VT300 / VT400 */

DiLo('?')	DiHi('?')	DiDi(0xbf)	/* Digital VT300 / VT400 */
DiLo('?')	DiHi('~')	DiDi(0xbf)

DiLo('@')	DiHi('A')	DiDi(0xc5)
DiLo('@')	DiHi('a')	DiDi(0xe5)

DiLo('A')	DiHi('A')	DiDi('@')	/* Digital VT300 / VT400 */
DiLo('A')	DiHi('E')	DiDi(0xc6)
DiLo('A')	DiHi('^')	DiDi(0xc2)
DiLo('A')	DiHi('_')	DiDi(0xaa)	/* Digital VT300 / VT400 */
DiLo('A')	DiHi('`')	DiDi(0xc0)
DiLo('A')	DiHi('~')	DiDi(0xc3)
DiLo('A')	DiHi(0xa8)	DiDi(0xc4)	/* Digital VT300 / VT400 */
DiLo('A')	DiHi(0xb0)	DiDi(0xc5)	/* Digital VT300 / VT400 */
DiLo('A')	DiHi(0xb4)	DiDi(0xc1)	/* Digital VT300 / VT400 */

DiLo('C')	DiHi('O')	DiDi(0xa9)	/* Digital VT300 / VT400 */
DiLo('C')	DiHi('|')	DiDi(0xa2)	/* Digital VT300 / VT400 */
DiLo('C')	DiHi(0xb8)	DiDi(0xc7)	/* Digital VT300 / VT400 */

DiLo('E')	DiHi('^')	DiDi(0xca)
DiLo('E')	DiHi('`')	DiDi(0xc8)
DiLo('E')	DiHi(0xa8)	DiDi(0xcb)	/* Digital VT300 / VT400 */
DiLo('E')	DiHi(0xb4)	DiDi(0xc9)	/* Digital VT300 / VT400 */

DiLo('H')	DiHi('T')	DiDi(0xde)	/* Digital VT300 / VT400 */

DiLo('I')	DiHi('^')	DiDi(0xce)
DiLo('I')	DiHi('`')	DiDi(0xcc)
DiLo('I')	DiHi(0xa8)	DiDi(0xcf)	/* Digital VT300 / VT400 */
DiLo('I')	DiHi(0xb4)	DiDi(0xcd)	/* Digital VT300 / VT400 */

DiLo('N')	DiHi('~')	DiDi(0xd1)

DiLo('O')	DiHi('R')	DiDi(0xae)	/* Digital VT300 / VT400 */
DiLo('O')	DiHi('S')	DiDi(0xa7)	/* Digital VT300 / VT400 */
DiLo('O')	DiHi('X')	DiDi(0xa4)	/* Digital VT300 / VT400 */
DiLo('O')	DiHi('^')	DiDi(0xd4)
DiLo('O')	DiHi('_')	DiDi(0xba)	/* Digital VT300 / VT400 */
DiLo('O')	DiHi('`')	DiDi(0xd2)
DiLo('O')	DiHi('~')	DiDi(0xd5)
DiLo('O')	DiHi(0xa8)	DiDi(0xd6)	/* Digital VT300 / VT400 */
DiLo('O')	DiHi(0xb4)	DiDi(0xd3)	/* Digital VT300 / VT400 */

DiLo('U')	DiHi('^')	DiDi(0xdb)
DiLo('U')	DiHi('`')	DiDi(0xd9)
DiLo('U')	DiHi(0xa8)	DiDi(0xdc)	/* Digital VT300 / VT400 */
DiLo('U')	DiHi(0xb4)	DiDi(0xda)	/* Digital VT300 / VT400 */

DiLo('Y')	DiHi(0xb4)	DiDi(0xdd)	/* Digital VT300 / VT400 */

DiLo('^')	DiHi('a')	DiDi(0xe2)
DiLo('^')	DiHi('e')	DiDi(0xea)
DiLo('^')	DiHi('i')	DiDi(0xee)
DiLo('^')	DiHi('o')	DiDi(0xf4)
DiLo('^')	DiHi('u')	DiDi(0xfb)

DiLo('`')	DiHi('a')	DiDi(0xe0)
DiLo('`')	DiHi('e')	DiDi(0xe8)
DiLo('`')	DiHi('i')	DiDi(0xec)
DiLo('`')	DiHi('o')	DiDi(0xf2)
DiLo('`')	DiHi('u')	DiDi(0xf9)

DiLo('a')	DiHi('e')	DiDi(0xe6)
DiLo('a')	DiHi('~')	DiDi(0xe3)
DiLo('a')	DiHi(0xa8)	DiDi(0xe4)	/* Digital VT300 / VT400 */
DiLo('a')	DiHi(0xb0)	DiDi(0xe5)	/* Digital VT300 / VT400 */
DiLo('a')	DiHi(0xb4)	DiDi(0xe1)	/* Digital VT300 / VT400 */

DiLo('c')	DiHi(0xb8)	DiDi(0xe7)	/* Digital VT300 / VT400 */

DiLo('e')	DiHi(0xa8)	DiDi(0xeb)	/* Digital VT300 / VT400 */
DiLo('e')	DiHi(0xb4)	DiDi(0xe9)	/* Digital VT300 / VT400 */

DiLo('h')	DiHi('t')	DiDi(0xfe)	/* Digital VT300 / VT400 */

DiLo('i')	DiHi(0xa8)	DiDi(0xef)	/* Digital VT300 / VT400 */
DiLo('i')	DiHi(0xb4)	DiDi(0xed)	/* Digital VT300 / VT400 */

DiLo('n')	DiHi('~')	DiDi(0xf1)

DiLo('o')	DiHi('~')	DiDi(0xf5)
DiLo('o')	DiHi(0xa8)	DiDi(0xf6)	/* Digital VT300 / VT400 */
DiLo('o')	DiHi(0xb4)	DiDi(0xf3)	/* Digital VT300 / VT400 */

DiLo('s')	DiHi('s')	DiDi(0xdf)	/* Digital VT300 / VT400 */

DiLo('u')	DiHi(0xa8)	DiDi(0xfc)	/* Digital VT300 / VT400 */
DiLo('u')	DiHi(0xb4)	DiDi(0xfa)	/* Digital VT300 / VT400 */

DiLo('x')	DiHi('x')	DiDi(0xd7)	/* Digital VT300 / VT400 */

DiLo('y')	DiHi(0xa8)	DiDi(0xff)	/* Digital VT300 / VT400 */

DiLo('|')	DiHi('|')	DiDi(0xa6)	/* Digital VT300 / VT400 */
