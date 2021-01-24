/*
 * Copyright (c) 2017 Rafal Lalik rafallalik@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "typewriter.h"
#include <string>
#include <cstring>
#include <sstream>
#include <cmath>

using namespace std;

/*======================================================================*
 TypeWriter parser related stuff
*======================================================================*/

const char macro_char = ':';
const char nextframe_char = ',';
const char nextstep_char = '>';
const char delkey_char = '<';
const char optbeg_char = '[';
const char optend_char = ']';
const char rangebeg_char = '{';
const char rangeend_char = '}';
const char escape_char = '\\';

std::string null_string;

TypeWriter::TypeWriter()
    : frame_rate(25)
    , frame_step(1)
    , parsing_err(0), last_used_idx(-1)
{
}


void TypeWriter::clear()
{
    frames.clear();
}

void TypeWriter::setPattern(const std::string& str)
{
    raw_string = str;
    frames.reserve(raw_string.length());
}

int TypeWriter::parse()
{
    clear();

    gen.seed(step_seed);
    d = std::normal_distribution<>{0, step_sigma};

    previous_total_frame = -1;
    int start_frame = 0;
    parsing_err = parseString(raw_string, start_frame);
    return parsing_err;
}

void TypeWriter::printParseResult()
{
    if (parsing_err < 0)
    {
        fprintf(stderr, "Parsing error:\n%.*s\n", -parsing_err-1, raw_string.c_str());
        fprintf(stderr, "%*c%c\n", -parsing_err-2, ' ', '^');
    }
    else
    {
        printf("Parsing OK:  frames=%u  strings=%ld\n", count(), frames.size());
    }
}

uint TypeWriter::count() const
{
    return frames.back().frame;
}


uint TypeWriter::getOrInsertFrame(uint frame)
{
    // create new or reuse old frame
    // by design last->frame cannot be larger than frame
    // take the last frame then FIXME: should we break parser here?

    int real_frame = frame * frame_step;

    uint n = frames.size();
    if (!n)
    {
        int s = std::round(d(gen));

        if ((s + real_frame) > 0)
            real_frame += s;
        if (real_frame <= previous_total_frame)
            real_frame = previous_total_frame+1;
        previous_total_frame = real_frame;

        frames.push_back(Frame(frame, real_frame));
        return 0;
    }

    if (frames[n-1].frame >= frame)
        return n-1;

    int s = std::round(d(gen));

    if ((s + real_frame) > 0)
        real_frame += s;
    if (real_frame <= previous_total_frame)
        real_frame = previous_total_frame+1;
    previous_total_frame = real_frame;


    Frame f = Frame(frame, real_frame);
    f.s = frames[n-1].s;
    frames.push_back(f);

    return n;
}

void TypeWriter::insertChar(char c, uint frame)
{
    char buff[2] = "\0";
    buff[0] = c;
    insertString(buff, frame);
}

void TypeWriter::insertString(const std::string & str, uint frame)
{
    uint n = getOrInsertFrame(frame);
    Frame & f = frames[n];
    f.s.append(str);
}

void TypeWriter::insertBypass(uint frame)
{
    uint n = getOrInsertFrame(frame);

    addBypass(n);
}

const std::string & TypeWriter::render(uint frame)
{
    uint n = frames.size();
    if (!n)
        return null_string;

    if (last_used_idx == -1)
        last_used_idx = 0;

    // start with current frame
    Frame f = frames[last_used_idx];

    // but if current is ahead 'frame', start from beginning
    if (f.real_frame > frame)
        last_used_idx = 0;

    if (frames[last_used_idx].real_frame > frame)
        return null_string;

    for (; last_used_idx < (int)n-1; ++last_used_idx)
    {
        f = frames[last_used_idx+1];
        if (f.real_frame > frame)
            return frames[last_used_idx].s;
    }

    return frames[last_used_idx].s;
}

void TypeWriter::addBypass(uint idx)
{
    if (idx == 0)
    {
        frames[idx].s.clear();
        return;
    }

    int pidx = -1;

    if (frames[idx].bypass == -2)
        pidx = idx-1;
    else
        pidx = frames[idx].bypass;

    if (pidx == -1)
        return;

    while (true)
    {
        if (frames[pidx].bypass != -2)
        {
            pidx = frames[pidx].bypass;
        }
        else
        {
            --pidx;
            break;
        }
    }
    frames[idx].bypass = pidx;

    if (frames[idx].bypass >= 0)
        frames[idx].s = frames[frames[idx].bypass].s;
    else
        frames[idx].s.clear();
}

Frame::Frame(uint frame, uint real_frame) : frame(frame), real_frame(real_frame), bypass(-2)
{
}

void Frame::print()
{
    printf("%c [%d] (%d) %s %c\n",
           true ? '-' : '|',
           frame, real_frame, s.c_str(),
           true ? '-' : '|');
}

std::string TypeWriter::detectUtf8(const std::string& str, size_t pos)
{
    /*
     * 0x00 do 0x7F            – bits 0xxxxxxx
     * 0x80 do 0x7FF           – bits 110xxxxx 10xxxxxx
     * 0x800 do 0xFFFF         – bits 1110xxxx 10xxxxxx 10xxxxxx
     * 0x10000 do 0x1FFFFF     – bits 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
     * 0x200000 do 0x3FFFFFF   – bits 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
     * 0x4000000 do 0x7FFFFFFF – bits 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
     */
    unsigned char c = str[pos];
    static const unsigned char masks[5] = { 0xfc, 0xf8, 0xf0, 0xc0, 0x80 };
    if ((c & 0x80) == 0x00)
    {
        return str.substr(pos, 1);
    }
    else
    {
        for (int i = 0; i < 5; ++i)
        {
            if ((c & masks[i]) == masks[i])
            {
                for (int j = 0; j < (4-i); ++j)
                {
                    if ( !(0x80 & str[pos+1+j]) )
                        return str.substr(pos, 1);
                }
                return str.substr(pos, 5-i);
            }
        }
    }
    return "";
}

struct TypeWriter::ParseOptions
{
    ParseOptions() : n(0), fskip(0), sskip(0) {}

    uint n;
    uint fskip;
    uint sskip;
};

uint TypeWriter::getFrameSkipFromOptions(const ParseOptions & po, bool steps)
{
    if (steps)
        return (po.n + po.sskip) * frame_rate;
    else
        return po.sskip * frame_rate + po.fskip + po.n;
}

int TypeWriter::parseString(const std::string& line, int start_frame)
{
    size_t limit = line.length();

    uint frame = start_frame;
    std::string frame_text;

    char check_for_options = 0;
    bool was_escaped = false;
    uint i = 0;
    while (i < limit)
    {
        was_escaped = false;
        check_for_options = 0;

        char c = line[i];
        if (c == escape_char)
        {
            ++i;
            c = line[i];
            if (!c)
                return -i-1;

            was_escaped = true;
        }
        else if (c == nextframe_char)
        {
            ++frame;    // increase frame number
            check_for_options = nextframe_char;
        }
        else if (c == nextstep_char)
        {
            frame += frame_rate;
            check_for_options = nextstep_char;
        }
        else if (c == macro_char)
        {
            ++i;
            int ret = parseMacro(line, i, frame);
            if (ret < 0)
                return ret;

            continue;
        }

        if (check_for_options)
        {
            // get next char and check whether it is an option
            ++i;
            c = line[i];

            ParseOptions po;
            int ret = parseOptions(line, i, po);
            if (ret < 0)
            {
                return ret;
            }

            uint n = getFrameSkipFromOptions(po, check_for_options == nextstep_char);
            if (check_for_options == nextframe_char)
            {
                if (n > 0)
                    frame += (n - 1);
            }
            else if (check_for_options == nextstep_char)
            {
                if (n > 0)
                    frame += (n - frame_rate);
            }

            continue;
        }

        // append values
        if (!was_escaped and c == delkey_char)
        {
            // get next char and check whether it is an option
            ++i;
            c = line[i];

            ParseOptions po;
            po.n = 1;
            int ret = parseOptions(line, i, po);
            if (ret < 0)
            {
                return ret;
            }

            for (uint i = 0; i < po.n; ++i)
                insertBypass(frame);
        }
        else
        {
            if (was_escaped)
            {
                if (c == 'n')
                    c = '\n';
                else if (c == 't')
                    c = '\t';
            }
            std::string test_str = detectUtf8(line, i);
            insertString(test_str, frame);
            i += test_str.length();
        }
    }

    return frame;
}

int TypeWriter::parseOptions(const std::string& line, uint & i, ParseOptions & po)
{
    char c = line[i];
    if (c != optbeg_char)
        return i;

    ++i;
    c = line[i];

    int n = 0;  // stores number of frames to skip

    while (c != optend_char and c)
    {
        // if is digit then add to frames skip number
        if (isdigit(c))
        {
            int v = c - 48; // quick conv from char to int
            n = n*10 + v;
        }
        // s if for seconds: mult frames by f. rate
        else if (c == 's')
        {
            po.sskip = n;
            n = 0;
        }
        // just frames
        else if (c == 'f')
        {
            po.fskip = n;
            n = 0;
        }
        else if (c == ',')
        {
            if (n)
                po.n = n;
        }
        else
        {
            // unknown char, return error
            return -i-1;
        }

        ++i;
        c = line[i];
    }

    if (n)
        po.n = n;

    ++i;

    return i;
}

int TypeWriter::parseMacro(const std::string& line, uint & i, uint & frame)
{
    std::vector<std::string> string_list;
    uint n = 0;
    char c = line[i];
    if (c == 'c')   // split by characters
    {
        ++i;

        // calculate skip from options
        ParseOptions po;
        int ret = parseOptions(line, i, po);
        if (ret < 0)
        {
            return ret;
        }
        n = getFrameSkipFromOptions(po);
        if (n == 0)
            n = 1;

        c = line[i];

        if (c != rangebeg_char)
            return -i-1;

        ++i;
        c = line[i];

        while (c != rangeend_char and c)
        {
            if (c == escape_char)
            {
                ++i;
                c = line[i];
                if (!c)
                    return -i-1;
            }

            std::string test_str = detectUtf8(line, i);
            insertString(test_str, frame);
            frame += n;

            i += test_str.length();
            c = line[i];
        }
    }
    else if (c == 'w')   // split by words
    {
        ++i;

        // calculate skip from options
        ParseOptions po;
        int ret = parseOptions(line, i, po);
        if (ret < 0)
        {
            return ret;
        }
        n = getFrameSkipFromOptions(po);
        if (n == 0)
            n = 1;

        c = line[i];

        if (c != rangebeg_char)
            return -i-1;

        ++i;
        c = line[i];

        size_t i_end = i;
        while (true)
        {
            // search for range end
            i_end = line.find_first_of(rangeend_char, i_end);

            // if end of string, error
            if (i_end == line.npos)
                return -i_end-1;

            // chack if endrange char is not escaped
            if (line[i_end-1] != escape_char or line[i_end-2] == escape_char)
                break;

            ++i_end;
        }

        std::string substr = line.substr(i, i_end-i);

        size_t pos = 0;
        while (true)
        {
            pos = substr.find_first_of(escape_char, pos);
            if (pos == substr.npos)
                break;
            substr.replace(pos, 1, "");
        }

        pos = 0;
        size_t pos2 = 0;
        std::string s;
        while (pos2 != substr.npos)
        {
            pos2 = substr.find_first_of(" \t\n", pos);
            if (pos2 != substr.npos)
            {
                pos2 = substr.find_first_not_of(" \t\n", pos2);
                if (pos2 != substr.npos)
                    s = substr.substr(pos, pos2-pos);
                else
                    s = substr.substr(pos, -1);
            }
            else
            {
                s = substr.substr(pos, -1);
            }

            insertString(s, frame);
            frame += n;
            pos = pos2;
        }
        i = i_end;
        ++i;
    }
    else if (c == 'l')   // split by lines
    {
        ++i;

        // calculate skip from options
        ParseOptions po;
        int ret = parseOptions(line, i, po);
        if (ret < 0)
        {
            return ret;
        }
        n = getFrameSkipFromOptions(po);
        if (n == 0)
            n = 1;

        c = line[i];

        if (c != rangebeg_char)
            return -i-1;

        ++i;
        c = line[i];

        size_t i_end = i;
        while (true)
        {
            // search for range end
            i_end = line.find_first_of(rangeend_char, i_end);

            // if end of string, error
            if (i_end == line.npos)
                return -i_end-1;

            // chack if endrange char is not escaped
            if (line[i_end-1] != escape_char or line[i_end-2] == escape_char)
                break;

            ++i_end;
        }

        std::string substr = line.substr(i, i_end-i);

        size_t pos = 0;
        while (true)
        {
            pos = substr.find_first_of(escape_char, pos);
            if (pos == substr.npos)
                break;
            substr.replace(pos, 1, "");
        }

        pos = 0;
        size_t pos2 = 0;
        std::string s;
        while (pos2 != substr.npos)
        {
            pos2 = substr.find_first_of("\n", pos);
            if (pos2 != substr.npos)
            {
                pos2 = substr.find_first_not_of("\n", pos2);
                if (pos2 != substr.npos)
                    s = substr.substr(pos, pos2-pos);
                else
                    s = substr.substr(pos, -1);
            }
            else
            {
                s = substr.substr(pos, -1);
            }

            insertString(s, frame);
            frame += n;
            pos = pos2;
        }
        i = i_end;
        ++i;
    }
    else    // error
    {
        return -i-1;
    }

    ++i;
    return i;
}

/*======================================================================*
 XML Parser related stuff
*======================================================================*/

char* clone_string(const char* string) {
    char* new_string = new char[strlen(string)+1];
    if (new_string)
        strcpy(new_string, string);
    return new_string;
}

XmlParser::XmlParser()
{
}

XmlParser::~XmlParser()
{
}

void XmlParser::clear()
{
}

void XmlParser::setDocument(const char * xml)
{
    clear();
    doc = QString::fromUtf8(xml);

    dom.setContent(doc);
    QDomElement title = dom.documentElement();

    items = title.elementsByTagName("item");
}

int XmlParser::parse()
{
    node_vec.clear();

    for (int i = 0; i < items.count(); ++i)
    {
        QDomNode node = items.item( i );
        QDomNamedNodeMap nodeAttributes = node.attributes();
        if ( nodeAttributes.namedItem( "type" ).nodeValue() == "QGraphicsTextItem" )
        {
            QDomNode lnode = node.namedItem( "content" ).firstChild();
            node_vec.push_back(lnode);
        }
    }

    return 1;
}

QString XmlParser::getNodeContent(uint i) const
{
    if (i >= node_vec.size())
        return nullptr;

    return node_vec[i].nodeValue();
}

void XmlParser::setNodeContent(uint i, const QString & content)
{
    if (i >= node_vec.size())
        return;

    node_vec[i].setNodeValue(content);
}

QString XmlParser::getDocument() const
{
    return dom.toString();
}
