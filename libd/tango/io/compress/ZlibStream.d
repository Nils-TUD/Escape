/*******************************************************************************

    copyright:  Copyright (C) 2007 Daniel Keep.  All rights reserved.

    license:    BSD style: $(LICENSE)

    author:     Daniel Keep

    version:    Feb 08: Added support for different stream encodings, removed
                        old "window bits" ctors.
                        
                Dec 07: Added support for "window bits", needed for Zip support.
                
                Jul 07: Initial release.

*******************************************************************************/

module tango.io.compress.ZlibStream;

private import tango.io.compress.c.zlib;

private import tango.stdc.stringz : fromStringz;

private import tango.core.Exception : IOException;

private import tango.io.device.Conduit : InputFilter, OutputFilter;

private import tango.io.model.IConduit : InputStream, OutputStream, IConduit;

private import tango.text.convert.Integer : toString;


/* This constant controls the size of the input/output buffers we use
 * internally.  This should be a fairly sane value (it's suggested by the zlib
 * documentation), that should only need changing for memory-constrained
 * platforms/use cases.
 *
 * An alternative would be to make the chunk size a template parameter to the
 * filters themselves, but Tango already has more than enough template
 * parameters getting in the way :)
 */

private enum { CHUNKSIZE = 256 * 1024 };

/* This constant specifies the default windowBits value.  This is taken from
 * documentation in zlib.h.  It shouldn't break anything if zlib changes to
 * a different default.
 */

private enum { WINDOWBITS_DEFAULT = 15 };

/*******************************************************************************
  
    This input filter can be used to perform decompression of zlib streams.

*******************************************************************************/

class ZlibInput : InputFilter
{
    /***************************************************************************
    
        This enumeration allows you to specify the encoding of the compressed
        stream.
    
    ***************************************************************************/

    enum Encoding : int
    {
        /**
         *  The code should attempt to automatically determine what the encoding
         *  of the stream should be.  Note that this cannot detect the case
         *  where the stream was compressed with no encoding.
         */
        Guess,
        /**
         *  Stream has zlib encoding.
         */
        Zlib,
        /**
         *  Stream has gzip encoding.
         */
        Gzip,
        /**
         *  Stream has no encoding.
         */
        None
    }

    private
    {
        /* Used to make sure we don't try to perform operations on a dead
         * stream. */
        bool zs_valid = false;

        z_stream zs;
        ubyte[] in_chunk;
    }
    
    /***************************************************************************

        Constructs a new zlib decompression filter.  You need to pass in the
        stream that the decompression filter will read from.  If you are using
        this filter with a conduit, the idiom to use is:

        ---
        auto input = new ZlibInput(myConduit.input));
        input.read(myContent);
        ---

        The optional windowBits parameter is the base two logarithm of the
        window size, and should be in the range 8-15, defaulting to 15 if not
        specified.  Additionally, the windowBits parameter may be negative to
        indicate that zlib should omit the standard zlib header and trailer,
        with the window size being -windowBits.
        
      Params:
        stream = compressed input stream.
        
        encoding =
            stream encoding.  Defaults to Encoding.Guess, which
            should be sufficient unless the stream was compressed with
            no encoding; in this case, you must manually specify
            Encoding.None.
            
        windowBits =
            the base two logarithm of the window size, and should be in the
            range 8-15, defaulting to 15 if not specified.

    ***************************************************************************/

    this(InputStream stream, Encoding encoding,
            int windowBits = WINDOWBITS_DEFAULT)
    {
        init(stream, encoding, windowBits);
        scope(failure) kill_zs();

        super(stream);
        in_chunk = new ubyte[CHUNKSIZE];
    }
    
    /// ditto
    this(InputStream stream)
    {
        // DRK 2009-02-26
        // Removed unique implementation in favour of passing on to another
        // constructor.  The specific implementation was because the default
        // value of windowBits is documented in zlib.h, but not actually
        // exposed.  Using inflateInit over inflateInit2 ensured we would
        // never get it wrong.  That said, the default value of 15 is REALLY
        // unlikely to change: values below that aren't terribly useful, and
        // values higher than 15 are already used for other purposes.
        // Also, this leads to less code which is always good.  :D
        this(stream, Encoding.init);
    }

    /*
     * This method performs initialisation for the stream.  Note that this may
     * be called more than once for an instance, provided the instance is
     * either new or as part of a call to reset.
     */
    private void init(InputStream stream, Encoding encoding, int windowBits)
    {
        /*
         * Here's how windowBits works, according to zlib.h:
         * 
         * 8 .. 15
         *      zlib encoding.
         *      
         * (8 .. 15) + 16
         *      gzip encoding.
         *      
         * (8 .. 15) + 32
         *      auto-detect encoding.
         *      
         * (8 .. 15) * -1
         *      raw/no encoding.
         *      
         * Since we're going to be playing with the value, we DO care whether
         * windowBits is in the expected range, so we'll check it.
         */
        if( !( 8 <= windowBits && windowBits <= 15 ) )
        {
            // No compression for you!
            throw new ZlibException("invalid windowBits argument"
                ~ .toString(windowBits));
        }
        
        switch( encoding )
        {
        case Encoding.Zlib:
            // no-op
            break;
            
        case Encoding.Gzip:
            windowBits += 16;
            break;

        case Encoding.Guess:
            windowBits += 32;
            break;
            
        case Encoding.None:
            windowBits *= -1;
            break;

        default:
            assert (false);
        }
        
        // Allocate inflate state
        with( zs )
        {
            zalloc = null;
            zfree = null;
            opaque = null;
            avail_in = 0;
            next_in = null;
        }

        auto ret = inflateInit2(&zs, windowBits);
        if( ret != Z_OK )
            throw new ZlibException(ret);

        zs_valid = true;

        // Note that this is redundant when init is called from the ctor, but
        // it is NOT REDUNDANT when called from reset.  source is declared in
        // InputFilter.
        //
        // This code is a wee bit brittle, since if the ctor of InputFilter
        // changes, this code might break in subtle, hard to find ways.
        //
        // See ticket #1837
        this.source = stream;
    }
    
    ~this()
    {
        if( zs_valid )
            kill_zs();
    }

    /***************************************************************************
        
        Resets and re-initialises this instance.

        If you are creating compression streams inside a loop, you may wish to
        use this method to re-use a single instance.  This prevents the
        potentially costly re-allocation of internal buffers.

        The stream must have already been closed before calling reset.

    ***************************************************************************/ 

    void reset(InputStream stream, Encoding encoding,
            int windowBits = WINDOWBITS_DEFAULT)
    {
        // If the stream is still valid, bail.
        if( zs_valid )
            throw new ZlibStillOpenException;
        
        init(stream, encoding, windowBits);
    }

    /// ditto

    void reset(InputStream stream)
    {
        reset(stream, Encoding.init);
    }

    /***************************************************************************

        Decompresses data from the underlying conduit into a target array.

        Returns the number of bytes stored into dst, which may be less than
        requested.

    ***************************************************************************/ 

    override size_t read(void[] dst)
    {
        if( !zs_valid )
            return IConduit.Eof;

        // Check to see if we've run out of input data.  If we have, get some
        // more.
        if( zs.avail_in == 0 )
        {
            auto len = source.read(in_chunk);
            if( len == IConduit.Eof )
                return IConduit.Eof;

            zs.avail_in = len;
            zs.next_in = in_chunk.ptr;
        }

        // We'll tell zlib to inflate straight into the target array.
        zs.avail_out = dst.length;
        zs.next_out = cast(ubyte*)dst.ptr;
        auto ret = inflate(&zs, Z_NO_FLUSH);

        switch( ret )
        {
            case Z_NEED_DICT:
                // Whilst not technically an error, this should never happen
                // for general-use code, so treat it as an error.
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                kill_zs();
                throw new ZlibException(ret);

            case Z_STREAM_END:
                // zlib stream is finished; kill the stream so we don't try to
                // read from it again.
                kill_zs();
                break;

            default:
        }

        return dst.length - zs.avail_out;
    }

    /***************************************************************************

        Closes the compression stream.

    ***************************************************************************/ 

    override void close()
    {
        // Kill the stream.  Don't deallocate the buffer since the user may
        // yet reset the stream.
        if( zs_valid )
            kill_zs();

        super.close();
    }

    // Disable seeking
    override long seek(long offset, Anchor anchor = Anchor.Begin)
    {
        throw new IOException("ZlibInput does not support seek requests");
    }

    // This function kills the stream: it deallocates the internal state, and
    // unsets the zs_valid flag.
    private void kill_zs()
    {
        check_valid();

        inflateEnd(&zs);
        zs_valid = false;
    }

    // Asserts that the stream is still valid and usable (except that this
    // check doesn't get elided with -release).
    private void check_valid()
    {
        if( !zs_valid )
            throw new ZlibClosedException;
    }
}

/*******************************************************************************
  
    This output filter can be used to perform compression of data into a zlib
    stream.

*******************************************************************************/

class ZlibOutput : OutputFilter
{
    /***************************************************************************

        This enumeration represents several pre-defined compression levels.

        Any integer between -1 and 9 inclusive may be used as a level,
        although the symbols in this enumeration should suffice for most
        use-cases.

    ***************************************************************************/

    enum Level : int
    {
        /**
         * Default compression level.  This is selected for a good compromise
         * between speed and compression, and the exact compression level is
         * determined by the underlying zlib library.  Should be roughly
         * equivalent to compression level 6.
         */
        Normal = -1,
        /**
         * Do not perform compression.  This will cause the stream to expand
         * slightly to accommodate stream metadata.
         */
        None = 0,
        /**
         * Minimal compression; the fastest level which performs at least
         * some compression.
         */
        Fast = 1,
        /**
         * Maximal compression.
         */
        Best = 9
    }

    /***************************************************************************
    
        This enumeration allows you to specify what the encoding of the
        compressed stream should be.
    
    ***************************************************************************/

    enum Encoding : int
    {
        /**
         *  Stream should use zlib encoding.
         */
        Zlib,
        /**
         *  Stream should use gzip encoding.
         */
        Gzip,
        /**
         *  Stream should use no encoding.
         */
        None
    }

    private
    {
        bool zs_valid = false;
        z_stream zs;
        ubyte[] out_chunk;
        size_t _written = 0;
    }

    /***************************************************************************

        Constructs a new zlib compression filter.  You need to pass in the
        stream that the compression filter will write to.  If you are using
        this filter with a conduit, the idiom to use is:

        ---
        auto output = new ZlibOutput(myConduit.output);
        output.write(myContent);
        ---

        The optional windowBits parameter is the base two logarithm of the
        window size, and should be in the range 8-15, defaulting to 15 if not
        specified.  Additionally, the windowBits parameter may be negative to
        indicate that zlib should omit the standard zlib header and trailer,
        with the window size being -windowBits.

    ***************************************************************************/

    this(OutputStream stream, Level level, Encoding encoding,
            int windowBits = WINDOWBITS_DEFAULT)
    {
        init(stream, level, encoding, windowBits);
        scope(failure) kill_zs();

        super(stream);
        out_chunk = new ubyte[CHUNKSIZE];
    }
    
    /// ditto
    this(OutputStream stream, Level level = Level.Normal)
    {
        // DRK 2009-02-26
        // Removed unique implementation in favour of passing on to another
        // constructor.  See ZlibInput.this(InputStream).
        this(stream, level, Encoding.init);
    }

    /*
     * This method performs initialisation for the stream.  Note that this may
     * be called more than once for an instance, provided the instance is
     * either new or as part of a call to reset.
     */
    private void init(OutputStream stream, Level level, Encoding encoding,
            int windowBits)
    {
        /*
         * Here's how windowBits works, according to zlib.h:
         * 
         * 8 .. 15
         *      zlib encoding.
         *      
         * (8 .. 15) + 16
         *      gzip encoding.
         *      
         * (8 .. 15) + 32
         *      auto-detect encoding.
         *      
         * (8 .. 15) * -1
         *      raw/no encoding.
         *      
         * Since we're going to be playing with the value, we DO care whether
         * windowBits is in the expected range, so we'll check it.
         * 
         * Also, note that OUR Encoding enum doesn't contain the 'Guess'
         * member.  I'm still waiting on tango.io.psychic...
         */
        if( !( 8 <= windowBits && windowBits <= 15 ) )
        {
            // No compression for you!
            throw new ZlibException("invalid windowBits argument"
                ~ .toString(windowBits));
        }
        
        switch( encoding )
        {
        case Encoding.Zlib:
            // no-op
            break;
            
        case Encoding.Gzip:
            windowBits += 16;
            break;
            
        case Encoding.None:
            windowBits *= -1;
            break;

        default:
            assert (false);
        }
        
        // Allocate deflate state
        with( zs )
        {
            zalloc = null;
            zfree = null;
            opaque = null;
        }

        auto ret = deflateInit2(&zs, level, Z_DEFLATED, windowBits, 8,
                Z_DEFAULT_STRATEGY);
        if( ret != Z_OK )
            throw new ZlibException(ret);

        zs_valid = true;

        // This is NOT REDUNDANT.  See ZlibInput.init.
        this.sink = stream;
    }

    ~this()
    {
        if( zs_valid )
            kill_zs();
    }

    /***************************************************************************
        
        Resets and re-initialises this instance.

        If you are creating compression streams inside a loop, you may wish to
        use this method to re-use a single instance.  This prevents the
        potentially costly re-allocation of internal buffers.

        The stream must have already been closed or committed before calling
        reset.

    ***************************************************************************/ 

    void reset(OutputStream stream, Level level, Encoding encoding,
            int windowBits = WINDOWBITS_DEFAULT)
    {
        // If the stream is still valid, bail.
        if( zs_valid )
            throw new ZlibStillOpenException;

        init(stream, level, encoding, windowBits);
    }

    /// ditto
    void reset(OutputStream stream, Level level = Level.Normal)
    {
        reset(stream, level, Encoding.init);
    }

    /***************************************************************************

        Compresses the given data to the underlying conduit.

        Returns the number of bytes from src that were compressed; write
        should always consume all data provided to it, although it may not be
        immediately written to the underlying output stream.

    ***************************************************************************/

    override size_t write(void[] src)
    {
        check_valid();
        scope(failure) kill_zs();

        zs.avail_in = src.length;
        zs.next_in = cast(ubyte*)src.ptr;

        do
        {
            zs.avail_out = out_chunk.length;
            zs.next_out = out_chunk.ptr;

            auto ret = deflate(&zs, Z_NO_FLUSH);
            if( ret == Z_STREAM_ERROR )
                throw new ZlibException(ret);

            // Push the compressed bytes out to the stream, until it's either
            // written them all, or choked.
            auto have = out_chunk.length-zs.avail_out;
            auto out_buffer = out_chunk[0..have];
            do
            {
                auto w = sink.write(out_buffer);
                if( w == IConduit.Eof )
                    return w;

                out_buffer = out_buffer[w..$];
                _written += w;
            }
            while( out_buffer.length > 0 );
        }
        // Loop while we are still using up the whole output buffer
        while( zs.avail_out == 0 );

        assert( zs.avail_in == 0, "failed to compress all provided data" );

        return src.length;
    }

    /***************************************************************************

        This read-only property returns the number of compressed bytes that
        have been written to the underlying stream.  Following a call to
        either close or commit, this will contain the total compressed size of
        the input data stream.

    ***************************************************************************/

    size_t written()
    {
        return _written;
    }

    /***************************************************************************

        Close the compression stream.  This will cause any buffered content to
        be committed to the underlying stream.

    ***************************************************************************/

    override void close()
    {
        // Only commit if the stream is still open.
        if( zs_valid ) commit;

        super.close;
    }

    /***************************************************************************

        Purge any buffered content.  Calling this will implicitly end the zlib
        stream, so it should not be called until you are finished compressing
        data.  Any calls to either write or commit after a compression filter
        has been committed will throw an exception.

        The only difference between calling this method and calling close is
        that the underlying stream will not be closed.

    ***************************************************************************/

    void commit()
    {
        check_valid();
        scope(failure) kill_zs();

        zs.avail_in = 0;
        zs.next_in = null;

        bool finished = false;

        do
        {
            zs.avail_out = out_chunk.length;
            zs.next_out = out_chunk.ptr;

            auto ret = deflate(&zs, Z_FINISH);
            switch( ret )
            {
                case Z_OK:
                    // Keep going
                    break;

                case Z_STREAM_END:
                    // We're done!
                    finished = true;
                    break;

                default:
                    throw new ZlibException(ret);
            }

            auto have = out_chunk.length - zs.avail_out;
            auto out_buffer = out_chunk[0..have];
            if( have > 0 )
            {
                do
                {
                    auto w = sink.write(out_buffer);
                    if( w == IConduit.Eof )
                        return;

                    out_buffer = out_buffer[w..$];
                    _written += w;
                }
                while( out_buffer.length > 0 );
            }
        }
        while( !finished );

        kill_zs();
    }

    // Disable seeking
    override long seek(long offset, Anchor anchor = Anchor.Begin)
    {
        throw new IOException("ZlibOutput does not support seek requests");
    }

    // This function kills the stream: it deallocates the internal state, and
    // unsets the zs_valid flag.
    private void kill_zs()
    {
        check_valid();

        deflateEnd(&zs);
        zs_valid = false;
    }

    // Asserts that the stream is still valid and usable (except that this
    // check doesn't get elided with -release).
    private void check_valid()
    {
        if( !zs_valid )
            throw new ZlibClosedException;
    }
}

/*******************************************************************************
  
    This exception is thrown if you attempt to perform a read, write or flush
    operation on a closed zlib filter stream.  This can occur if the input
    stream has finished, or an output stream was flushed.

*******************************************************************************/

class ZlibClosedException : IOException
{
    this()
    {
        super("cannot operate on closed zlib stream");
    }
}

/*******************************************************************************
  
    This exception is thrown if you attempt to reset a compression stream that
    is still open.  You must either close or commit a stream before it can be
    reset.

*******************************************************************************/

class ZlibStillOpenException : IOException
{
    this()
    {
        super("cannot reset an open zlib stream");
    }
}

/*******************************************************************************
  
    This exception is thrown when an error occurs in the underlying zlib
    library.  Where possible, it will indicate both the name of the error, and
    any textural message zlib has provided.

*******************************************************************************/

class ZlibException : IOException
{
    /*
     * Use this if you want to throw an exception that isn't actually
     * generated by zlib.
     */
    this(char[] msg)
    {
        super(msg);
    }
    
    /*
     * code is the error code returned by zlib.  The exception message will
     * be the name of the error code.
     */
    this(int code)
    {
        super(codeName(code));
    }

    /*
     * As above, except that it appends msg as well.
     */
    this(int code, char* msg)
    {
        super(codeName(code)~": "~fromStringz(msg));
    }

    protected char[] codeName(int code)
    {
        char[] name;

        switch( code )
        {
            case Z_OK:              name = "Z_OK";              break;
            case Z_STREAM_END:      name = "Z_STREAM_END";      break;
            case Z_NEED_DICT:       name = "Z_NEED_DICT";       break;
            case Z_ERRNO:           name = "Z_ERRNO";           break;
            case Z_STREAM_ERROR:    name = "Z_STREAM_ERROR";    break;
            case Z_DATA_ERROR:      name = "Z_DATA_ERROR";      break;
            case Z_MEM_ERROR:       name = "Z_MEM_ERROR";       break;
            case Z_BUF_ERROR:       name = "Z_BUF_ERROR";       break;
            case Z_VERSION_ERROR:   name = "Z_VERSION_ERROR";   break;
            default:                name = "Z_UNKNOWN";
        }

        return name;
    }
}

/* *****************************************************************************

    This section contains a simple unit test for this module.  It is hidden
    behind a version statement because it introduces additional dependencies.

***************************************************************************** */

debug(UnitTest) {

import tango.io.device.Array : Array;

void check_array(char[] FILE=__FILE__, int LINE=__LINE__)(
        ubyte[] as, ubyte[] bs, lazy char[] msg)
{
    assert( as.length == bs.length,
        FILE ~":"~ toString(LINE) ~ ": " ~ msg()
        ~ "array lengths differ (" ~ toString(as.length)
        ~ " vs " ~ toString(bs.length) ~ ")" );
    
    foreach( i, a ; as )
    {
        auto b = bs[i];
        
        assert( a == b,
            FILE ~":"~ toString(LINE) ~ ": " ~ msg()
            ~ "arrays differ at " ~ toString(i)
            ~ " (" ~ toString(cast(int) a)
            ~ " vs " ~ toString(cast(int) b) ~ ")" );
    }
}

unittest
{
    // One ring to rule them all, one ring to find them,
    // One ring to bring them all and in the darkness bind them.
    const char[] message = 
        "Ash nazg durbatulûk, ash nazg gimbatul, "
        "ash nazg thrakatulûk, agh burzum-ishi krimpatul.";
    
    static assert( message.length == 90 );

    // This compressed data was created using Python 2.5's built in zlib
    // module, with the default compression level.
    {
        const ubyte[] message_z = [
            0x78,0x9c,0x73,0x2c,0xce,0x50,0xc8,0x4b,
            0xac,0x4a,0x57,0x48,0x29,0x2d,0x4a,0x4a,
            0x2c,0x29,0xcd,0x39,0xbc,0x3b,0x5b,0x47,
            0x21,0x11,0x26,0x9a,0x9e,0x99,0x0b,0x16,
            0x45,0x12,0x2a,0xc9,0x28,0x4a,0xcc,0x46,
            0xa8,0x4c,0xcf,0x50,0x48,0x2a,0x2d,0xaa,
            0x2a,0xcd,0xd5,0xcd,0x2c,0xce,0xc8,0x54,
            0xc8,0x2e,0xca,0xcc,0x2d,0x00,0xc9,0xea,
            0x01,0x00,0x1f,0xe3,0x22,0x99];
    
        scope cond_z = new Array(2048);
        scope comp = new ZlibOutput(cond_z);
        comp.write (message);
        comp.close;
    
        assert( comp.written == message_z.length );
        
        /+
        Stdout("message_z:").newline;
        foreach( b ; cast(ubyte[]) cond_z.slice )
            Stdout.format("0x{0:x2},", b);
        Stdout.newline.newline;
        +/
    
        //assert( message_z == cast(ubyte[])(cond_z.slice) );
        check_array!(__FILE__,__LINE__)
            ( message_z, cast(ubyte[]) cond_z.slice, "message_z " );
    
        scope decomp = new ZlibInput(cond_z);
        auto buffer = new ubyte[256];
        buffer = buffer[0 .. decomp.read(buffer)];
    
        //assert( cast(ubyte[])message == buffer );
        check_array!(__FILE__,__LINE__)
            ( cast(ubyte[]) message, buffer, "message (zlib) " );
    }
    
    // This compressed data was created using the Cygwin gzip program
    // with default options.  The original file was called "testdata.txt".
    {
        const ubyte[] message_gz = [
            0x1f,0x8b,0x08,0x00,0x80,0x70,0x6f,0x45,
            0x00,0x03,0x73,0x2c,0xce,0x50,0xc8,0x4b,
            0xac,0x4a,0x57,0x48,0x29,0x2d,0x4a,0x4a,
            0x2c,0x29,0xcd,0x39,0xbc,0x3b,0x5b,0x47,
            0x21,0x11,0x26,0x9a,0x9e,0x99,0x0b,0x16,
            0x45,0x12,0x2a,0xc9,0x28,0x4a,0xcc,0x46,
            0xa8,0x4c,0xcf,0x50,0x48,0x2a,0x2d,0xaa,
            0x2a,0xcd,0xd5,0xcd,0x2c,0xce,0xc8,0x54,
            0xc8,0x2e,0xca,0xcc,0x2d,0x00,0xc9,0xea,
            0x01,0x00,0x45,0x38,0xbc,0x58,0x5a,0x00,
            0x00,0x00];
        
        // Compresses the original message, and outputs the bytes.  You can use
        // this to test the output of ZlibOutput with gzip.  If you use this,
        // don't forget to import Stdout somewhere.
        /+
        scope comp_gz = new Array(2048);
        scope comp = new ZlibOutput(comp_gz, ZlibOutput.Level.Normal, ZlibOutput.Encoding.Gzip, WINDOWBITS_DEFAULT);
        comp.write(message);
        comp.close;
        
        Stdout.format("message_gz ({0} bytes):", comp_gz.slice.length).newline;
        foreach( b ; cast(ubyte[]) comp_gz.slice )
            Stdout.format("0x{0:x2},", b);
        Stdout.newline;
        +/
        
        // We aren't going to test that we can compress to a gzip stream
        // since gzip itself always adds stuff like the filename, timestamps,
        // etc.  We'll just make sure we can DECOMPRESS gzip streams.
        scope decomp_gz = new Array(message_gz.dup);
        scope decomp = new ZlibInput(decomp_gz);
        auto buffer = new ubyte[256];
        buffer = buffer[0 .. decomp.read(buffer)];
        
        //assert( cast(ubyte[]) message == buffer );
        check_array!(__FILE__,__LINE__)
            ( cast(ubyte[]) message, buffer, "message (gzip) ");
    }
}
}
