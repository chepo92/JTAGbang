// jtag bang - serial JTAG interface for the Arduino
// pesco 2013, ISC license

#define VERSION     "0.1"
#define ATTRIBUTION "pesco 2013"

#define ACTIVE HIGH     // level of logic 1 on the JTAG lines
#define QUIET 0         // set to 1 to remove startup banner
#define XONXOFF 0       // send software flow control characters?
#define TCKWAIT 1       // ms delay between clock edges


#define LED 13

#define TMS 8
#define TDI 9
#define TDO 10
#define TCK 11

// optional hardware flow control
#define CTS 2   // connect this pin to the FTDI's CTS line
                // on the Arduino Duemilanove that is "X3-1"
                // for the Arduino Uno (no FTDI), i don't know
                // NB: this is assumed active-low


// JTAG pin functions
void tms(boolean value);
boolean tdi(boolean value);
uint8_t tdin(int n, uint8_t bits);
void tck(void);

void set(uint8_t pin, boolean value);
boolean get(uint8_t pin);

// output encoding
char out1(boolean value);
char out4(uint8_t value);

// flow control
void xon(void);
void xoff(void);
void waitforinput(void);

// subfunctions
void enumdevs(void);


const char *help =
    "type the following characters to affect stuff:\r\n"
    "! set TMS=1 then pulse TCK       . pulse TCK then set TDI=0\r\n"
    "* set TMS=0 then pulse TCK       , pulse TCK then set TDI=1\r\n"
    "                             [0-f] 4-bit shorthand (TDI)\r\n"
    "X enumerate devices\r\n"
    "? print this banner\r\n"
    "\r\n"
    "e.g. !!!!!*!!*ff!!    (reset, load BYPASS instruction)\r\n"
    "     !!!!!*!*00000000 (reset, read IDCODE register)\r\n"
    "\r\n"
    "linefeed sends, output (TDO) appears in same format.";

void banner(void)
{
    Serial.println();
    Serial.println("jtag bang v" VERSION ", " ATTRIBUTION);
    Serial.println();
    Serial.print(help);
    Serial.print("\r\n\n"); // to skip over the banner, wait for this
    Serial.flush();
}

void setup()
{
    pinMode(LED, OUTPUT);

    pinMode(TMS, OUTPUT);
    pinMode(TDI, OUTPUT);   // our output, their input
    pinMode(TDO, INPUT);    // their output, our input
    pinMode(TCK, OUTPUT);

    pinMode(CTS, OUTPUT);

    Serial.begin(9600);

#if QUIET == 0
    banner();
#endif
}


#define BUFSIZE 128
char buf[BUFSIZE];

void loop()
{
    int i, n=0;
    char c='\0';

    // read input until buffer full or end of line
    while(n<BUFSIZE && c!='\n') {
        waitforinput();
        c = Serial.read();
        if(c == '\r')
            c = '\n';   // normalize

        if(c == '\b' && n > 0)
            n--;
        else
            buf[n++] = c;
    }

    // handle each character; replace buf with output
    for(i=0; i<n; i++) {
        char c = buf[i];
        switch(c) {
        case '!':   tms(1); c=' ';      break;
        case '*':   tms(0); c=' ';      break;
        case '.':   c=out1(tdi(0));     break;
        case ',':   c=out1(tdi(1));     break;
        case 'X':   enumdevs(); c='\r'; break;
        case '?':   banner(); c='\r';   break;
        default:
            if(isdigit(c) || (c>='a' && c<='f')) {
                // hex notation
                uint8_t v;
                v = isdigit(c)? (c - '0') : (10 + c - 'a');
                c = out4(tdin(4, v));
            } else if(isspace(c)) {
                // pass to keep visual alignment
            } else {
                c = '-';
            }
        }
        buf[i] = c;
    }

    // write output
    for(i=0; i<n; i++) {
        char c = buf[i];
        if(c == '\n')
            Serial.println();   // "\r\n"
        else
            Serial.print(c);
    }
    Serial.flush();
}



// subfunction: enumerate the JTAG chain
void enumdevs(void)
{
    uint8_t n7, n6, n5, n4, n3, n2, lsb;

    // Test-Logic-Reset
    tms(1); tms(1); tms(1); tms(1); tms(1);

    // Capture-DR
    tms(0); tms(1); tms(0);

    for(;;) {
        lsb = tdi(1);
        if(lsb == 0) {
            Serial.println("0         [device has no idcode]");
        } else {
            // read manufacturer code (7+4 bits)
            lsb |= tdin(7, 0xff) << 1;
            n2 = tdin(4, 0xf);

            if(lsb == 0xff && n2 == 0xf) {
                // no such manufacturer, must be our own 1s
                // NB: actually IEEE 1149.1-2001 reserves 0000 1111111
                //     as the invalid manufacturer code, but i'm too
                //     lazy to track when to insert the zeros, so let's
                //     assume 1111 1111111 will not be used either.
                break;
            }

            // read product code
            n3 = tdin(4, 0xf);
            n4 = tdin(4, 0xf);
            n5 = tdin(4, 0xf);
            n6 = tdin(4, 0xf);
            // read product version
            n7 = tdin(4, 0xf);

            // print in the format VPPPPMMM
            Serial.print(out4(n7));
            Serial.print(out4(n6));
            Serial.print(out4(n5));
            Serial.print(out4(n4));
            Serial.print(out4(n3));
            Serial.print(out4(n2));
            Serial.print(out4(lsb >> 4));
            Serial.print(out4(lsb & 0xf));

            // print in the format vvvv pppppppppppppppp mmmmmmmmmmm 1
            Serial.print("  [");
            printbits(4, n7);
            Serial.print(' ');
            printbits(4, n6);
            printbits(4, n5);
            printbits(4, n4);
            printbits(4, n3);
            Serial.print(' ');
            printbits(4, n2);
            printbits(7, lsb >> 1);
            Serial.print(' ');
            printbits(1, lsb);
            Serial.print("]");

            Serial.println();
        }
    }

    // Run-Test/Idle
    tms(1); tms(1); tms(0);
}


// set TMS to value, cycle TCK
void tms(boolean value)
{
    set(TMS, value);
    tck();
}

// cycle TCK, set TDI to value, sample TDO
boolean tdi(boolean value)
{
    tck();
    set(TDI, value);
    return get(TDO);
}

// multi-bit version of tdi()
uint8_t tdin(int n, uint8_t bits)
{
    uint8_t tmp=0, res=0;
    int i;

    // shift bits and push into tmp lifo-order
    for(i=0; i<n; i++) {
        tmp = tmp<<1 | tdi(bits & 1);
        bits >>= 1;
    }

    // reverse bit order tmp->res
    for(i=0; i<n; i++) {
        res = res<<1 | tmp&1;
        tmp >>= 1;
    }

    return res;
}

void tck(void)
{
    set(TCK, 1);
    delay(TCKWAIT);
    set(TCK, 0);
    delay(TCKWAIT);
}

void set(uint8_t pin, boolean value)
{
    digitalWrite(pin, value? ACTIVE : !ACTIVE);
}

boolean get(uint8_t pin)
{
    return (digitalRead(pin) == ACTIVE);
}

char out1(boolean value)
{
    return (value? ',' : '.');
}

char out4(uint8_t value)
{
    if(value<10)    
        return ('0' + value);
    else
        return ('a' + value - 10);
}

void printbits(int n, uint8_t bits)
{
    boolean b;

    for(n--; n>=0; n--) {
        b = (bits >> n) & 1;
        Serial.print(b? '1' : '0');
    }
}

void xon(void)
{
#if XONXOFF == 1
    Serial.print('\x11');           // DC1
#endif
    digitalWrite(CTS, LOW);         // clear to send
    digitalWrite(LED, HIGH);
}

void xoff(void)
{
#if XONXOFF == 1
    Serial.print('\x13');           // DC3
#endif
    digitalWrite(CTS, HIGH);        // not clear to send
    digitalWrite(LED, LOW);
}

void waitforinput(void)
{
    if(!Serial.available()) {
        xon();
        while(!Serial.available()) {}
        xoff();
    }
}
