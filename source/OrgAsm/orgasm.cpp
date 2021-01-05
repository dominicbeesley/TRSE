#include "orgasm.h"
#include <QRegularExpression>

bool OrgasmLine::m_inIFDEF = false;

void Orgasm::LoadFile(QString filename) {
    LoadCodes(m_cpuFlavor);
    QFile f(filename);
    f.open(QFile::ReadOnly);
    m_source=f.readAll();

    ProcessSource();

    f.close();
    m_lines = m_source.split("\n");
    m_pCounter = 0;
    m_done = false;
    m_constantPassLines = -1;
}

void Orgasm::LoadCodes(int CPUFlavor)
{
    QString filename = ":/resources/text/opcodes.txt";
    if (CPUFlavor==2) filename = ":/resources/text/opcodes_GS4510.txt";
    QFile f(filename);

//    int num_states[3] {11,11,13};

    f.open(QFile::ReadOnly);
    QString all = f.readAll();
   // qDebug() << filename << all;
    f.close();
    for (QString s: all.split("\n")) {
        s = s.trimmed().simplified().toLower();
        if (s=="") continue;
        if (s.startsWith("#")) continue;
        QStringList lst = s.split(",");
        QVector<uchar> opCodes;
        for (int i=0;i<lst.count()-1;i++) {
            bool ok;
            int code = lst[i+1].toInt(&ok, 16);
            opCodes.append( code );
        }
        MOSOperandCycle c = m_opCodes[lst[0]];

        c.m_opcodes = opCodes;
        m_opCodes.m_opCycles[lst[0]] = c;
    }

}

void Orgasm::ProcessSource()
{
  //  QElapsedTimer t;
    //t.start();
    for (int i=0;i<256;i++) {
        QString r = "#P"+QString::number(i)+";";
//        m_source = m_source.replace(r,QChar(i));
        m_source = m_source.replace(r,"\"," + QString::number(i) + ",\"");
    }
//    qDebug() << "Orgasm::Processources took " << Util::MilisecondToString(t.elapsed());
}

OrgasmLine Orgasm::LexLine(int i) {
    OrgasmLine l;
    QString line = m_lines[i];
    // First: Do we have alabel?
//    line = line.replace("//",";");
    line = line.replace("\t", " ");
    line = line.replace("dc.b", ".byte");
    line = line.replace("!by", ".byte");
    line = line.replace("!fi", ".byte");
    line = line.replace("dc.w", ".word");
    line = line.replace(" EQU ", " = ");
    //line = line.replace("ds.w", ".word");
 //   line = line.replace("equ", "=");
//    line = line.replace("EQU", "=");
    //line = line.replace("ds.b", ".byte");
    line = line.replace(".dc ", ".byte ");

    if (line.simplified().contains("; LineNumber: ")) {
        m_curRasLine = line.split(":").last().toInt();
    }
    l.m_rasLineNumber = m_curRasLine;

    line = line.split(";")[0];
    if (!line.toLower().contains("incbin"))
        line = line.split("//")[0];

    l.m_orgLine = line;
    if (line=="") {
        l.m_ignore = true;
        return l;

    }

//    if (line.toLower().contains("else"))
  //      qDebug() << line;

    line.replace("("," (");

    if (line.simplified().toLower().startsWith("ifconst")) {
        OrgasmLine::m_inIFDEF= true;
        l.m_ignore = true;
        return l;

    }

    if (line.simplified().toLower().startsWith("ifnconst")) {
        OrgasmLine::m_inIFDEF= false;
        l.m_ignore = true;
        return l;

    }


    if (line.simplified().toLower().startsWith("endif")) {
        OrgasmLine::m_inIFDEF= false;
        l.m_ignore = true;
        return l;

    }

    if (line.simplified().toLower() == ("else")) {
        OrgasmLine::m_inIFDEF= !OrgasmLine::m_inIFDEF;
        l.m_ignore = true;
        return l;

    }

    if (OrgasmLine::m_inIFDEF) {
        l.m_ignore = true;
        return l;

    }

//    qDebug() << line.contains("\"") << line;
    if (!(line[0]==" ") && !(line.contains("=") && !line.contains("\""))) {
        QString lbl = line.replace(":", " ").trimmed().split(" ")[0];
        l.m_label = lbl;
        line.remove(0, lbl.length()).replace(":", " ");

    }

    if (line.simplified().startsWith(";") || line.simplified()=="" || line.simplified().startsWith("//")) {
        if (l.m_label=="")
            l.m_ignore = true;
        else l.m_type = OrgasmLine::LABELONLY;
        return l;
    }


    QStringList lst = line.simplified().split(" ");
//    qDebug() << line.trimmed();
    if (line.trimmed().toLower().startsWith("incbin")) {
        lst.clear();
        lst.append("incbin");
        lst.append(line.simplified().remove(0,6).trimmed());

    }

    if (lst[0].toLower()=="org") {
        l.m_type = OrgasmLine::ORG;
        l.m_expr = "";
        for (int i=0;i<lst.count()-1;i++)
           l.m_expr += lst[i+1] + " ";


        return l;
    }
    if (lst[0].toLower()=="incbin") {
        l.m_type = OrgasmLine::INCBIN;
        l.m_expr = lst[1];
        return l;
    }
    if (line.contains("=") && !line.contains("\"")) {
        l.m_type = OrgasmLine::CONSTANT;
        QStringList cl = line.split("=");
        l.m_label = cl[0].trimmed();
        l.m_expr = cl[1].trimmed();
        if (l.m_label.toLower()=="x" ||  l.m_label.toLower()=="y") {
            throw OrgasmError("Orgasm does not support constants (or absolute addresses) that uses the name 'x' or 'y'! Please use a different name.",l);
        }
        //qDebug() << l.m_expr;
        return l;
    }
    if (lst[0].toLower()==".byte") {
        l.m_type = OrgasmLine::BYTE;
        l.m_expr = line.replace(".byte", "").trimmed();//.simplified();
        return l;
    }
    if (lst[0].toLower()=="ds.b") {
        l.m_type = OrgasmLine::PADBYTE;
        l.m_expr = line.replace("ds.b", "").trimmed();//.simplified();
        return l;
    }
    if (lst[0].toLower()==".word") {
        l.m_type = OrgasmLine::WORD;
        l.m_expr = line.replace(".word", "").trimmed();//.simplified();
        return l;
    }


    l.m_type = OrgasmLine::INSTRUCTION;
    l.m_instruction.Init(lst);
/*    if (lst.count()>1)
        l.m_expr = lst[1];*/
    l.m_expr="";
    for (int i=0;i<lst.count()-1;i++)
        l.m_expr +=lst[i+1] + " ";

    //        l.m.l.m_instruction.getTypeFromParams(lst[1]);
    //      l.m_op



    return l;
}
/*
bool stringSort(const QString &s1, const QString &s2)
{
    return s1.count() < s2.count();
}
*/
bool Orgasm::Assemble(QString filename, QString outFile)
{
    m_success = false;
    LoadFile(filename);
    m_olines.clear();

    try {

    for (int i=0;i<m_lines.count();i++) {
        if (m_lines[i]=="") continue;
        OrgasmLine ol = LexLine(i);
        ol.m_lineNumber = i;
        if (!ol.m_ignore)
            m_olines.append(ol);
    }

    emit EmitTick(" [constants] ");
    PassFindConstants();
    PassReplaceConstants();

//       for (QString s: m_constants.keys())
  //         qDebug() << s << " " << m_constants[s];
   //     exit(1);
/*    while (m_constantPassLines!=0) {
        PassConstants();
        qDebug() << "PAssing" << m_constantPassLines;
    }
*/
    QTime myTimer;
    myTimer.start();
//    qDebug() << "LABELS  " << QString::number((myTimer.elapsed()/100.0));
    emit EmitTick(" [labels] ");
    Compile(OrgasmData::PASS_LABELS);

    emit EmitTick(" [symbols] ");
    Compile(OrgasmData::PASS_SYMBOLS);

    if (!m_hasOverlappingError)
        m_success = true;

    if (QFile::exists(outFile))
        QFile::remove(outFile);
    if (m_success) {
        QFile out(outFile);
        out.open(QFile::WriteOnly);
        out.write(m_data);
        out.close();
    }
    m_output = "Complete.";


    }
    catch (OrgasmError e) {
        //qDebug() << "Error compiling: " << e;
        m_output = "Error during OrgAsm assembly : " +e.msg;
        m_output +="<br><br>Line "+QString::number(e.oline.m_lineNumber+1)+ " in " +"<font color=\"orange\">"+filename+" :</font> "" : " +e.oline.m_orgLine;
    }
/*    qDebug() << "ORGASM : ";
    for (int i:m_lineAddress.keys()) {
        qDebug() << i << Util::numToHex(m_lineAddress[i]);
    }
*/
    return m_success;
}

void Orgasm::PassReplaceConstants()
{
    //    if (m_constantPassLines==0)
    for (OrgasmLine& ol : m_olines) {
        if (ol.m_type != OrgasmLine::CONSTANT) {
            for (QString k : m_constList) {
                if (ol.m_expr.contains(k))
                ol.m_expr =  OrgasmData::ReplaceWord(ol.m_expr, k, m_constants[k]);
            }

        }
    }

}



void Orgasm::PassFindConstants()
{
//    if (m_constantPassLines==0)
  //      return;
    m_constantPassLines = 0;
    for (OrgasmLine& ol : m_olines) {
        if (ol.m_type == OrgasmLine::CONSTANT) {
//            qDebug() << ol.m_label;
            if (m_constants.contains(ol.m_label)) {
                throw OrgasmError("OrgAsm error: constant '"+ol.m_label+"' already defined.",ol);
//                continue;
            }

            m_constants[ol.m_label] = ol.m_expr;
            m_constList.append(ol.m_label);

            for (QString key : m_constants.keys()) {
                ol.m_expr.replace(key, m_constants[key]);
            }

            long var = 0;
            //qDebug() << ol.m_expr;
            OrgasmData::BinopExpr(ol.m_expr,var,"");


            /*bool ok = Util::NumberFromStringHex(ol.m_expr, var);
            if (ok) {
                m_symbols[ol.m_label] = var;
            }
            else {
                m_constantPassLines++;
            }
            */
/*            qDebug() << ol.m_label << " = " << ol.m_expr;
            if (m_symbols.contains(ol.m_label))
                qDebug() << "NOW DEFINED : " << Util::numToHex(m_symbols[ol.m_label]);
*/
            //m_symbols[ol.m_label.simplified()] = ol.m_expr.simplified();
            //m_symbols[ol.m_label.simplified()] = ol.m_expr.simplified();
        }
    }

}

void Orgasm::Compile(OrgasmData::PassType pt)
{
    m_pCounter = 0;
//    return;
    for (QString k:m_constants.keys())
        m_constList.append(k);


    m_data = QByteArray();
    for (OrgasmLine& ol : m_olines) {
        if (ol.m_type == OrgasmLine::CONSTANT)
            continue;

        if (ol.m_label!="" && pt == OrgasmData::PASS_LABELS) {
//            qDebug() << "Writing to label: " << ol.m_label << Util::numToHex(m_pCounter);
            if (m_constants.contains(ol.m_label)) {
                throw OrgasmError("OrgAsm error: label '"+ol.m_label+"' already defined as a constant.",ol);

            }
            if (!m_symbols.contains(ol.m_label))
            {
                m_symbols[ol.m_label] = m_pCounter;
                m_symbolsList.append(ol.m_label);
            }
            else {
                throw OrgasmError("OrgAsm error: symbol '"+ol.m_label+"' already defined",ol);
            }
            if (!ol.m_label.toLower().contains("endblock"))
                m_prevLabel = ol.m_label;
        }
        ol.m_pos = m_pCounter;
        if (pt == OrgasmData::PASS_LABELS && ol.m_rasLineNumber!=0) {
            bool ok = true;
            if (m_lineAddress.contains(ol.m_rasLineNumber)) {
                ok = false;
                if (m_lineAddress[ol.m_rasLineNumber]==0) ok=true;
            }
            if (ok)
                m_lineAddress[ol.m_rasLineNumber] = ol.m_pos;

        }

//        qDm_pCounter

        if (ol.m_type==OrgasmLine::BYTE)
            ProcessByteData(ol,pt);

        if (ol.m_type==OrgasmLine::PADBYTE)
            ProcessBytePad(ol);

        if (ol.m_type==OrgasmLine::WORD)
            ProcessWordData(ol);

        if (ol.m_type==OrgasmLine::ORG)
            ProcessOrgData(ol);

      if (ol.m_type==OrgasmLine::INSTRUCTION)
            ProcessInstructionData(ol, pt);

        if (ol.m_type==OrgasmLine::INCBIN)
            ProcessIncBin(ol);


//        qDebug() << "End: "<< ol.m_instruction.m_opCode << ol.m_expr  << Util::numToHex(m_pCounter);

    }


}

void Orgasm::ProcessByteData(OrgasmLine &ol,OrgasmData::PassType pt)
{
    if (ol.m_expr.trimmed()=="") {
        m_pCounter++;
        m_data.append((char)0x00);
        return;
    }
//    qDebug() << ol.m_expr;



    QStringList lst = Util::fixStringListSplitWithCommaThatContainsStrings(ol.m_expr.split(","));

    for (QString s: lst) {

        if (s.trimmed()=="") continue;
        //      qDebug() << Util::NumberFromStringHex(s);
        if (!s.contains("\"")) {

            if (pt==OrgasmData::PASS_SYMBOLS) {
                //            qDebug() << "Before "<< s;
                for (QString& c: m_symbolsList)
                    if (s.contains(c)) {
                        //                  qDebug() << c;
                        //exit(1);
                        s = OrgasmData::ReplaceWord(s,c,Util::numToHex(m_symbols[c]));

                    }
                //              qDebug() << "After: "<<s;

                s = Util::numToHex(Util::NumberFromStringHex(s));
                //                qDebug() << "After2: "<<s;

            }


//            if (lst.count()>1)  qDebug() << "Adding: " <<  Util::NumberFromStringHex(s);
            m_data.append(Util::NumberFromStringHex(s));

//            qDebug() << Util::NumberFromStringHex(s);
            //qDebug() << s << Util::NumberFromStringHex(s);
            m_pCounter++;

        }
        else {
            QString str = s;
            str = str.remove("\"");
            for (int i=0;i<str.length();i++) {
                int c = str.at(i).toLatin1();
                m_data.append((uchar)c);
                m_pCounter++;

            }
  //          exit(1);

        }
    }
    //    qDebug() << "Final val added: " << QString::number((uchar)m_data[m_data.count()-1]);
}

void Orgasm::ProcessBytePad(OrgasmLine &ol)
{
    QStringList lst = ol.m_expr.simplified().split(" ");

//    qDebug() << lst;
  //


//    qDebug() << lst;
    //qDebug() << ol.m_expr;
    int v = 0;
    if (lst.count()==2)
        v = Util::NumberFromStringHex(lst[1]);

    int cnt = Util::NumberFromStringHex(lst[0]);
    for (int i=0;i<cnt;i++) {
        m_pCounter++;
        m_data.append((char)v);

    }

}

void Orgasm::ProcessWordData(OrgasmLine &ol)
{
    if (ol.m_expr=="") {
        m_pCounter+=2;
        m_data.append((char)0x00);
        m_data.append((char)0x00);
        return;
    }
    QStringList lst = ol.m_expr.split(",");
    for (QString s: lst) {
        if (s.trimmed()=="") continue;
        s = s.trimmed().simplified();
        if (m_symbolsList.contains(s)) {
            m_data.append(m_symbols[s]&0xFF);
            m_data.append((m_symbols[s]>>8)&0xFF);
        }
        else {
            m_data.append(Util::NumberFromStringHex(s)&0xFF);
            m_data.append(Util::NumberFromStringHex(s)>>8);
        }
        m_pCounter++;
        m_pCounter++;
    }
}

void Orgasm::ProcessOrgData(OrgasmLine &ol)
{
    if (m_pCounter == 0) {
        int val = Util::NumberFromStringHex(ol.m_expr);
        m_data.append(val&0xFF);
        m_data.append((val>>8)&0xFF);
        m_pCounter = val;
        return;
    }
    else {


        for (QString s: m_symbols.keys())
                ol.m_expr = OrgasmData::ReplaceWord(ol.m_expr,s,Util::numToHex(m_symbols[s]));

        ol.m_expr =ol.m_expr.replace("#","");

        int val = Util::NumberFromStringHex(Util::BinopString(ol.m_expr));
        if (val<m_pCounter) {
            QString e = "Origin reversed index. Trying to move program counter backwards to '"+Util::numToHex(val)+"' from current counter " + Util::numToHex(m_pCounter)+".";
            e+="<br>Overlap concerns label : <b><font color=\"#FFFF20\">"+ m_prevLabel+"</font></b>";
            e+="<br><font color=\"#FF7020\">Oh no! You have included/defined overlapping data!</font><br><font color=\"#60FFFF\">Please make sure that your included data does not overlap, and make use of the memory analyzer tool!</font>";
            error = OrgasmError(e,ol);
            m_hasOverlappingError  = true;
//            throw OrgasmError(e,ol);
        }

        while (m_pCounter<val) {
            m_pCounter++;
//            m_data.append((uchar)0xff);
            m_data.append((uchar)0xff);
        }
    }
}

void Orgasm::ProcessIncBin(OrgasmLine &ol)
{
    //qDebug() << ol.m_expr;
    QString fn = ol.m_expr.replace("\"","");
    if (!QFile::exists(fn))
        throw OrgasmError("Could not include binary file: "+fn,ol);
    QFile f(fn);
    f.open(QFile::ReadOnly);
    QByteArray data = f.readAll();
    f.close();
    m_data.append(data);
    m_pCounter+=data.count();


}

void Orgasm::ProcessInstructionData(OrgasmLine &ol, OrgasmData::PassType pd)
{
//    QByteArray d = ol.m_instruction.Assemble(ol.m_expr, m_opCodes, pd, m_symbols, m_pCounter,m_constants, m_regs, m_symbolsList);

    QString expr = ol.m_expr;
    QString orgexpr = expr;
    QByteArray data;
    ol.m_instruction.m_opCode = ol.m_instruction.m_opCode.toLower();

    if (ol.m_instruction.m_opCode=="processor")
        return;
//    qDebug() << line;

    if (ol.m_instruction.m_opCode=="org")
        return;


    if (expr.contains("*")) {
//        line = line.replace("*", Util::numToHex(pCounter));
        QString add = expr;//expr.simplified().split(" ")[1].replace(" ", "");
        add = add.replace("*", Util::numToHex(m_pCounter));
        add = Util::BinopString(add);
        expr = add;//expr.split(" ")[0] + " " + add;
    //    qDebug() << "* : " << expr;

    }
    QString orgExpr = expr;
//    if (pass==OrgasmData::PASS_SYMBOLS)
    {

        QStringList l2 = expr.simplified().split(",");

        for (QString& c : m_constList) {
            l2[0] = OrgasmData::ReplaceWord(l2[0],c,m_constants[c]);

        }


        QString tst = l2[0];
        QString org = l2[0];
        tst = tst.replace("#", "").simplified();
        int cur = 0;
        if (!tst.startsWith("$"))
        for (QString& sym : m_symbolsList)
        {
//            qDebug() << "Testing for symbol " << sym << tst;
            if (!tst.contains(sym))
                continue;

            int i = m_symbols[sym];
            if (tst==("<"+sym)) {
                l2[0] = l2[0].replace("<"+sym,"$"+QString::number(i&0xFF,16));
                break;
            }
            else
                if (tst==(">"+sym)) {
                    l2[0] = l2[0].replace(">"+sym,"$"+QString::number((i>>8)&0xFF,16));
                    break;
                }

                l2[0] = OrgasmData::ReplaceWord(l2[0],sym,"$"+QString::number(i,16));
                if (org!=l2[0]) {
                    m_symbolsList.move(cur,0);
                    break;
                }
                cur++;
        }

        expr = l2[0];
        if (l2.count()>1)
            expr+=","+l2[1];

//        qDebug() << "ORGASM " <<expr << org << ol.m_expr;

        if (expr!="") {
            long val = 0;
            QString repl = "$1000";
            if (expr.contains("<") || expr.contains(">"))
                repl= "#$10";
            QString oexpr = expr;
            expr = expr.replace("<","").replace(">","");
            if (!(expr.startsWith("(") && expr.endsWith(")"))) {
                bool hasHash = false;
                if (expr.simplified().startsWith("#"))
                    hasHash = true;
                expr = OrgasmData::BinopExpr(oexpr, val, repl);
                if (hasHash)
                    expr = "#"+expr;
            }

            if (val==-1 && pd==OrgasmData::PASS_SYMBOLS) {
                throw OrgasmError("Unknown operation: " +orgExpr,ol);
            }

        }
    }





//    qDebug() << " ** " << line;

    QString m_opCode = ol.m_instruction.m_opCode;




    OrgasmInstruction::Type type = OrgasmInstruction::imp;
    MOSOperandCycle cyc;

//    if (opCode=="org")
  //      return false;

    if (m_opCodes.m_opCycles.contains(m_opCode))
        cyc = m_opCodes.m_opCycles[m_opCode];
    else {
        throw OrgasmError("Unknown opcode: " + m_opCode,ol);
    }

    if (cyc.m_opcodes.count()==0)
        throw OrgasmError("Opcode illegal or not implemented yet: '" + m_opCode + "' on line '" + orgexpr+"'",ol);


    if (expr!="")
        type =  ol.m_instruction.getTypeFromParams(expr);
    else
        type = OrgasmInstruction::none;



    // Override types or pass symbol
//    if (pass==OrgasmData::PASS_LABELS) {
/*        if (m_opCode == "jmp" || m_opCode=="jsr")
            type = OrgasmInstruction::abs;
*/
    if ((int)type>cyc.m_opcodes.length())
        throw OrgasmError("Unknown or non-implemented opcode: " + m_opCode,ol);

    int code = cyc.m_opcodes[(int)type];


    if (code==0 && pd==OrgasmData::PASS_SYMBOLS) {
        qDebug() << "ERROR on line : " << m_opCode + " " +expr;
        throw OrgasmError("Opcode type not implemented or illegal: " + m_opCode + "  type " +type + "        on line " + ol.m_expr,ol );
    }



    int val=0;
    if (type!=OrgasmInstruction::none) {
        QString num = expr.split(",")[0].replace("#","").replace("(","").replace(")","");
        val = Util::NumberFromStringHex(num);
    }


    data.append(code);


    if (m_opCode=="bpl" || m_opCode=="bne" || m_opCode=="beq" || m_opCode=="bcc" || m_opCode=="bcs" || m_opCode=="bvc" || m_opCode=="bmi" || m_opCode=="bvc" || m_opCode=="bvs") {
        int diff = (val)-m_pCounter-2;
        if (abs(diff)>=128 && pd==OrgasmData::PASS_SYMBOLS) {
            throw OrgasmError("Branch out of range : " +QString::number(diff) + " :" + m_opCode + " " +expr + " on line " + QString::number(ol.m_lineNumber),ol);
        }
        data.append((uchar)diff);
    }
    else

    if (type==OrgasmInstruction::zp || type==OrgasmInstruction::zpx || type==OrgasmInstruction::zpy || type == OrgasmInstruction::izx || type == OrgasmInstruction::izy || type == OrgasmInstruction::izz|| type==OrgasmInstruction::imm) {
        data.append(val);
    }
    else
    if (type==OrgasmInstruction::abs || type==OrgasmInstruction::abx || type == OrgasmInstruction::aby || type == OrgasmInstruction::ind) {
        data.append(val&0xFF);
        data.append((val>>8)&0xFF);
    }
    if (pd != OrgasmData::PASS_SYMBOLS)
        expr = orgExpr;




    if (data.count()>0) {
        m_data.append(data);
        m_pCounter+=data.count();
    }
}

void Orgasm::SaveSymbolsList(QString filename)
{
    if (QFile::exists(filename))
        QFile::remove(filename);

    QFile file( filename );
    QMap<int, bool> isSet;
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream( &file );
        // First breakpoints

        stream<<"; labels" << endl;
        for (QString s: m_symbolsList) {
            if (!s.startsWith("trse_breakpoint"))
                if (!isSet[m_symbols[s]]) {
                    stream << "al  " << Util::numToHex(m_symbols[s]) << " ."<< s << endl;
                    isSet[m_symbols[s]]=true;
                }
        }
        for (QString s: m_extraSymbols.keys()) {
            stream << "al  " << m_extraSymbols[s] << " ."<< s << endl;
        }

/*        for (QString s: m_constList) {
            if (!s.startsWith("trse_breakpoint"))
                    if (m_constants[s].startsWith("$"))
                        stream << "al  " << m_constants[s] << " ."<< s << endl;

        }
*/

        stream<<"; breakpoints" << endl;
        for (QString s: m_symbolsList) {
            if (s.startsWith("trse_breakpoint")) {
                stream << "break " << Util::numToHex(m_symbols[s])<<endl;
            }
        }


    }
}










QString OrgasmData::ReplaceWord(QString& line, QString& word, QString replacement)
{
    if (line==word)
        return replacement;
    if (!line.contains(word))
        return line;

    QRegularExpression rg("\\b" + word + "\\b");
    return line.replace(rg,replacement);

    return line;
}

QString OrgasmData::BinopExpr(QString& expr, long& val, QString rep)
{
    bool hasHash = false;
/*    if (expr.contains("#")) {
//        qDebug() << expr;
        hasHash = true;
        expr = expr.replace("#","");
    }*/
    QStringList l = expr.simplified().split(",");
    l[0] = l[0].replace("(","").replace(")","");
    l[0] = l[0].replace(" ","");
    //sym = sym.toLower();
//                qDebug() << "bf: " << l2[0] << "          expr: " << expr;
    l[0] =Util::BinopString(l[0]);


    if(!Util::NumberFromStringHex(l[0],val)) {
        val=-1;
        if (rep!="")
            l[0] = rep;
    }

    QString pa="";
    QString pb="";
    if (expr.contains("(")) {
        pa="(";
        pb=")";
    }
    expr = pa+l[0]+pb;
    if (l.count()>1)
        expr+="," + l[1];


/*    if (hasHash)
        expr = "#"+expr;*/
    return expr;
}
