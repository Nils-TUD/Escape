/*******************************************************************************

        copyright:      Copyright (c) 2008 Jeff Davey. All rights reserved

        license:        BSD style: $(LICENSE)

        author:         Jeff Davey

        standards:      rfc3548, rfc2045

        Since:          0.99.7

*******************************************************************************/

/*******************************************************************************

    This module is used to decode and encode base64 char[] arrays. 

    Example:
    ---
    char[] blah = "Hello there, my name is Jeff.";
    scope encodebuf = new char[allocateEncodeSize(cast(ubyte[])blah)];
    char[] encoded = encode(cast(ubyte[])blah, encodebuf);

    scope decodebuf = new ubyte[encoded.length];
    if (cast(char[])decode(encoded, decodebuf) == "Hello there, my name is Jeff.")
        Stdout("yay").newline;
    ---

*******************************************************************************/

module tango.util.encode.Base64;

/*******************************************************************************

    calculates and returns the size needed to encode the length of the 
    array passed.

    Params:
    data = An array that will be encoded

*******************************************************************************/


uint allocateEncodeSize(ubyte[] data)
{
    return allocateEncodeSize(data.length);
}

/*******************************************************************************

    calculates and returns the size needed to encode the length passed.

    Params:
    length = Number of bytes to be encoded

*******************************************************************************/

uint allocateEncodeSize(uint length)
{
    size_t tripletCount = length / 3;
    uint tripletFraction = length % 3;
    return (tripletCount + (tripletFraction ? 1 : 0)) * 4; // for every 3 bytes we need 4 bytes to encode, with any fraction needing an additional 4 bytes with padding
}


/*******************************************************************************

    encodes data into buff and returns the number of bytes encoded.
    this will not terminate and pad any "leftover" bytes, and will instead
    only encode up to the highest number of bytes divisible by three.

    returns the number of bytes left to encode

    Params:
    data = what is to be encoded
    buff = buffer large enough to hold encoded data
    bytesEncoded = ref that returns how much of the buffer was filled

*******************************************************************************/

int encodeChunk(ubyte[] data, char[] buff, ref int bytesEncoded)
{
    size_t tripletCount = data.length / 3;
    int rtn = 0;
    char *rtnPtr = buff.ptr;
    ubyte *dataPtr = data.ptr;

    if (data.length > 0)
    {
        rtn = tripletCount * 3;
        bytesEncoded = tripletCount * 4;
        for (size_t i; i < tripletCount; i++)
        {
            *rtnPtr++ = _encodeTable[((dataPtr[0] & 0xFC) >> 2)];
            *rtnPtr++ = _encodeTable[(((dataPtr[0] & 0x03) << 4) | ((dataPtr[1] & 0xF0) >> 4))];
            *rtnPtr++ = _encodeTable[(((dataPtr[1] & 0x0F) << 2) | ((dataPtr[2] & 0xC0) >> 6))];
            *rtnPtr++ = _encodeTable[(dataPtr[2] & 0x3F)];
            dataPtr += 3;
        }
    }

    return rtn;
}

/*******************************************************************************

    encodes data and returns as an ASCII base64 string.

    Params:
    data = what is to be encoded
    buff = buffer large enough to hold encoded data

    Example:
    ---
    char[512] encodebuf;
    char[] myEncodedString = encode(cast(ubyte[])"Hello, how are you today?", encodebuf);
    Stdout(myEncodedString).newline; // SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==
    ---


*******************************************************************************/

char[] encode(ubyte[] data, char[] buff)
in
{
    assert(data);
    assert(buff.length >= allocateEncodeSize(data));
}
body
{
    char[] rtn = null;

    if (data.length > 0)
    {
        int bytesEncoded = 0;
        int numBytes = encodeChunk(data, buff, bytesEncoded);
        char *rtnPtr = buff.ptr + bytesEncoded;
        ubyte *dataPtr = data.ptr + numBytes;
        int tripletFraction = data.length - (dataPtr - data.ptr);

        switch (tripletFraction)
        {
            case 2:
                *rtnPtr++ = _encodeTable[((dataPtr[0] & 0xFC) >> 2)];
                *rtnPtr++ = _encodeTable[(((dataPtr[0] & 0x03) << 4) | ((dataPtr[1] & 0xF0) >> 4))];
                *rtnPtr++ = _encodeTable[((dataPtr[1] & 0x0F) << 2)];
                *rtnPtr++ = '=';
                break;
            case 1:
                *rtnPtr++ = _encodeTable[((dataPtr[0] & 0xFC) >> 2)];
                *rtnPtr++ = _encodeTable[((dataPtr[0] & 0x03) << 4)];
                *rtnPtr++ = '=';
                *rtnPtr++ = '=';
                break;
            default:
                break;
        }
        rtn = buff[0..(rtnPtr - buff.ptr)];
    }

    return rtn;
}

/*******************************************************************************

    encodes data and returns as an ASCII base64 string.

    Params:
    data = what is to be encoded

    Example:
    ---
    char[] myEncodedString = encode(cast(ubyte[])"Hello, how are you today?");
    Stdout(myEncodedString).newline; // SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==
    ---


*******************************************************************************/


char[] encode(ubyte[] data)
in
{
    assert(data);
}
body
{
    auto rtn = new char[allocateEncodeSize(data)];
    return encode(data, rtn);
}

/*******************************************************************************

    decodes an ASCCI base64 string and returns it as ubyte[] data. Pre-allocates
    the size of the array.

    This decoder will ignore non-base64 characters. So:
    SGVsbG8sIGhvd
    yBhcmUgeW91IH
    RvZGF5Pw==

    Is valid.

    Params:
    data = what is to be decoded

    Example:
    ---
    char[] myDecodedString = cast(char[])decode("SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==");
    Stdout(myDecodeString).newline; // Hello, how are you today?
    ---

*******************************************************************************/

ubyte[] decode(char[] data)
in
{
    assert(data);
}
body
{
    auto rtn = new ubyte[data.length];
    return decode(data, rtn);
}

/*******************************************************************************

    decodes an ASCCI base64 string and returns it as ubyte[] data.

    This decoder will ignore non-base64 characters. So:
    SGVsbG8sIGhvd
    yBhcmUgeW91IH
    RvZGF5Pw==

    Is valid.

    Params:
    data = what is to be decoded
    buff = a big enough array to hold the decoded data

    Example:
    ---
    ubyte[512] decodebuf;
    char[] myDecodedString = cast(char[])decode("SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==", decodebuf);
    Stdout(myDecodeString).newline; // Hello, how are you today?
    ---

*******************************************************************************/
       
ubyte[] decode(char[] data, ubyte[] buff)
in
{
    assert(data);
}
body
{
    ubyte[] rtn;

    if (data.length > 0)
    {
        ubyte[4] base64Quad;
        ubyte *quadPtr = base64Quad.ptr;
        ubyte *endPtr = base64Quad.ptr + 4;
        ubyte *rtnPt = buff.ptr;
        size_t encodedLength = 0;

        ubyte padCount = 0;
        ubyte endCount = 0;
        ubyte paddedPos = 0;
        foreach_reverse(char piece; data)
        {
            paddedPos++;
            ubyte current = _decodeTable[piece];
            if (current || piece == 'A')
            {
                endCount++;
                if (current == BASE64_PAD)
                    padCount++;
            }
            if (endCount == 4)
                break;
        }

        if (padCount > 2)
            throw new Exception("Improperly terminated base64 string. Base64 pad character (=) found where there shouldn't be one.");
        if (padCount == 0)
            paddedPos = 0;

        char[] nonPadded = data[0..(length - paddedPos)];
        foreach(piece; nonPadded)
        {
            ubyte next = _decodeTable[piece];
            if (next || piece == 'A')
                *quadPtr++ = next;
            if (quadPtr is endPtr)
            {
                rtnPt[0] = cast(ubyte) ((base64Quad[0] << 2) | (base64Quad[1] >> 4));
                rtnPt[1] = cast(ubyte) ((base64Quad[1] << 4) | (base64Quad[2] >> 2));
                rtnPt[2] = cast(ubyte) ((base64Quad[2] << 6) | base64Quad[3]);
                encodedLength += 3;
                quadPtr = base64Quad.ptr;
                rtnPt += 3;
            }
        }

        // this will try and decode whatever is left, even if it isn't terminated properly (ie: missing last one or two =)
        if (paddedPos)
        {
            char[] padded = data[(length - paddedPos) .. length];
            foreach(char piece; padded)
            {
                ubyte next = _decodeTable[piece];
                if (next || piece == 'A')
                    *quadPtr++ = next;
                if (quadPtr is endPtr)
                {
                    *rtnPt++ = cast(ubyte) (((base64Quad[0] << 2) | (base64Quad[1]) >> 4));
                    if (base64Quad[2] != BASE64_PAD)
                    {
                        *rtnPt++ = cast(ubyte) (((base64Quad[1] << 4) | (base64Quad[2] >> 2)));
                        encodedLength += 2;
                        break;
                    }
                    else
                    {
                        encodedLength++;
                        break;
                    }
                }
            }
        }

        rtn = buff[0..encodedLength];
    }

    return rtn;
}

version (Test)
{
    import tango.scrapple.util.Test;
    import tango.io.device.File;
    import tango.time.StopWatch;
    import tango.io.Stdout;

    unittest    
    {
        Test.Status encodeChunktest(ref char[][] messages)
        {
            char[] str = "Hello, how are you today?";
            char[] encoded = new char[allocateEncodeSize(cast(ubyte[])str)];
            int bytesEncoded = 0;
            int numBytesLeft = encodeChunk(cast(ubyte[])str, encoded, bytesEncoded);
            char[] result = encoded[0..bytesEncoded] ~ encode(cast(ubyte[])str[numBytesLeft..$], encoded[bytesEncoded..$]);
            if (result == "SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==")
                return Test.Status.Success;
            return Test.Status.Failure;
        }
        Test.Status encodeTest(ref char[][] messages)
        {
            char[] encoded = new char[allocateEncodeSize(cast(ubyte[])"Hello, how are you today?")];
            char[] result = encode(cast(ubyte[])"Hello, how are you today?", encoded);
            if (result == "SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==")
            {
                char[] result2 = encode(cast(ubyte[])"Hello, how are you today?");
                if (result == "SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==")
                    return Test.Status.Success;
            }

            return Test.Status.Failure;
        }

        Test.Status decodeTest(ref char[][] messages)
        {
            ubyte[1024] decoded;
            ubyte[] result = decode("SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==", decoded);
            if (result == cast(ubyte[])"Hello, how are you today?")
            {
                result = decode("SGVsbG8sIGhvdyBhcmUgeW91IHRvZGF5Pw==");
                if (result == cast(ubyte[])"Hello, how are you today?")
                    return Test.Status.Success;
            }
            return Test.Status.Failure;
        }
        
        Test.Status speedTest(ref char[][] messages)
        {
            Stdout("Reading...").newline;
            char[] data = cast(char[])File.get ("blah.b64");
            ubyte[] result = new ubyte[data.length];
            auto t1 = new StopWatch();
            Stdout("Decoding..").newline;
            t1.start();
            uint runs = 100000000;
            for (uint i = 0; i < runs; i++)
                decode(data, result);
            double blah = t1.stop();
            Stdout.formatln("Decoded {} MB in {} seconds at {} MB/s", cast(double)(cast(double)(data.length * runs) / 1024 / 1024), blah, (cast(double)(data.length * runs)) / 1024 / 1024 / blah );
            return Test.Status.Success;
        }

        Test.Status speedTest2(ref char[][] messages)
        {
            Stdout("Reading...").newline;
//            ubyte[] data = cast(ubyte[])FileData("blah.txt").read;
            ubyte[] data = cast(ubyte[])"I am a small string, Wee...";
            char[] result = new char[allocateEncodeSize(data)];
            auto t1 = new StopWatch();
            uint runs = 100000000;
            Stdout("Encoding..").newline;
            t1.start();
            for (uint i = 0; i < runs; i++)
                encode(data, result);
            double blah = t1.stop();
            Stdout.formatln("Encoded {} MB in {} seconds at {} MB/s", cast(double)(cast(double)(data.length * runs) / 1024 / 1024), blah, (cast(double)(data.length * runs)) / 1024 / 1024 / blah );
            return Test.Status.Success;
        }

        auto t = new Test("tango.util.encode.Base64");
        t["Encode"] = &encodeTest;
        t["Encode Stream"] = &encodeChunktest;
        t["Decode"] = &decodeTest;
//        t["Speed"] = &speedTest;
//        t["Speed2"] = &speedTest2;
        t.run();
    }
}



private:

/*
    Static immutable tables used for fast lookups to 
    encode and decode data.
*/
static const ubyte BASE64_PAD = 64;
static const char[] _encodeTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static const ubyte[] _decodeTable = [
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,62,0,0,0,63,52,53,54,55,56,57,58,
    59,60,61,0,0,0,BASE64_PAD,0,0,0,0,1,2,3,
    4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
    19,20,21,22,23,24,25,0,0,0,0,0,0,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0
];

