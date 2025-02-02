#include "dialogcolors.h"
#include "ui_dialogcolors.h"
#include "source/Compiler/syntax.h"


DialogColors::DialogColors(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogColors)
{
    ui->setupUi(this);
}

DialogColors::~DialogColors()
{
    delete m_lst;
    delete ui;
}

void DialogColors::Initialize(LImage *img, LColorList *lc, QString path) {
    m_org = lc;
    m_img = img;
    m_lst = new LColorList();
    m_lst->CopyFrom(m_org);
    //        m_lst->m_list = m_org->m_list;
    toGUI();
    m_projectPath = path;
}

void DialogColors::toGUI()
{
    m_lst->FillComboBox(ui->cbmColors);
    ui->cbmColors->setCurrentIndex(m_curCol);
    ui->cbmBitplanes->setCurrentIndex(m_lst->getNoBitplanes()-1);
    if (m_curCol!=0)
        setColor(m_curCol,0);

}


void DialogColors::setColor(int cc, int ty) {
    m_curCol = cc;
    QPushButton* button = ui->btnColor;
    QPalette pal = button->palette();
    pal.setColor(QPalette::Button, m_lst->get(m_curCol).color);
    button->setAutoFillBackground(true);
    button->setPalette(pal);
    button->update();

    if (ty==0) {
        ui->leRGB->setText(m_lst->get(m_curCol).toRGB8());
        ui->leRGB4->setText(m_lst->get(m_curCol).toRGB4());
    }
    if (ty==1)
        ui->leRGB->setText(m_lst->get(m_curCol).toRGB8());
    if (ty==2)
        ui->leRGB4->setText(m_lst->get(m_curCol).toRGB4());

    ui->lblRGB4->setText("$"+QString::number(m_lst->get(m_curCol).get12BitValue(),16));

}

void DialogColors::on_pushButton_clicked()
{
    //m_org->m_list = m_lst->m_list;
//    m_org->CopyFrom(m_lst);
    // WTF? Why does everything crash if you change color 0?
    for (int i=1;i<m_org->m_list.count();i++) {
//        qDebug() << m_org->m_list[i].color<<m_lst->m_list[i].color;
        m_img->m_colorList.m_list[i].color = m_lst->m_list[i].color;

    }
//    m_org->m_list = m_lst->m_list;
    if (Syntax::s.m_currentSystem->m_system==AbstractSystem::SNES ) {
        m_org->setNoBitplanes(m_lst->m_bpp.x());
    }

    close();
}

void DialogColors::on_cbmColors_activated(int index)
{
    setColor(index,0);
}

void DialogColors::on_leRGB_textChanged(const QString &arg1)
{
    m_lst->get(m_curCol).fromRGB8(arg1);
    m_lst->FillComboBox(ui->cbmColors);
    ui->cbmColors->setCurrentIndex(m_curCol);
    setColor(m_curCol,3);
}

void DialogColors::on_leRGB4_textChanged(const QString &arg1)
{
    m_lst->get(m_curCol).fromRGB4(arg1);
    m_lst->FillComboBox(ui->cbmColors);
    ui->cbmColors->setCurrentIndex(m_curCol);
    setColor(m_curCol,3);

}

void DialogColors::on_btnLoad_clicked()
{

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Import palette (from flf image)", m_projectPath,
                                                    "*.flf");
    if (fileName == "")
        return;

    QString f= fileName;

    if (!QFile::exists(f))
        return;

    if (f.toLower().endsWith(".flf")) {
        LImage* img = LImageIO::Load(f);

        if (m_img!=nullptr && img!=nullptr) {
                            qDebug() << "HER " << f;
            m_lst->CopyFrom(&img->m_colorList);
        }

    }


}

void DialogColors::on_btnSave_2_clicked()
{
    QString ext = "bin";
    QString ttr  = "Export amiga/atari ST palette";
    QString f = "Binary( *."+ext+" )";

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    ttr.toStdString().c_str(), m_projectPath,
                                                    f);


//    m_work.m_currentImage->m_image->ExportAsm(fileName);
//    MultiColorImage* mi = (MultiColorImage*)dynamic_cast<MultiColorImage*>(m_work.m_currentImage->m_image);

   // fileName = fileName.remove(".bin");
    if (Syntax::s.m_currentSystem->m_system == AbstractSystem::AMIGA)
        m_lst->ExportAmigaPalette(fileName);
    if (Syntax::s.m_currentSystem->m_system == AbstractSystem::ATARI520ST)
        m_lst->ExportAtariSTPalette(fileName);

}

void DialogColors::on_pushButton_2_clicked()
{
    close();

}

void DialogColors::on_cbmBitplanes_currentIndexChanged(int index)
{
    if (Syntax::s.m_currentSystem->m_processor == AbstractSystem::M68000 ||
       Syntax::s.m_currentSystem->m_system==AbstractSystem::SNES ) {
        m_lst->setNoBitplanes(index+1);

    }

    toGUI();
}

void DialogColors::on_btnLoad_2_clicked()
{
    QString pal = ui->cbmPalette->currentText();
    if (pal.toLower()=="greyscale") {
        m_lst->SetGreyscale(Util::fromColor(m_lst->getPenColour(1)),false);
    }
    if (pal.toLower()=="twocolors") {
        m_lst->SetTwoColors(Util::fromColor(m_lst->getPenColour(1)),
                            Util::fromColor(m_lst->getPenColour(2)));
    }
}
