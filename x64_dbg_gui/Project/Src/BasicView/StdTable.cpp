#include "StdTable.h"
#include "Bridge.h"


/*
 * TODO
 * Pass all variables in 32/64bits
 *
 */


StdTable::StdTable(QWidget *parent) : AbstractTableView(parent)
{
    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mIsMultiSelctionAllowed = false;

    mData = new QList< QList<QString>* >();

    mCopyMenuOnly = false;
    mCopyMenuDebugOnly = true;
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(Bridge::getBridge(), SIGNAL(repaintTableView()), this, SLOT(reloadData()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequestedSlot(QPoint)));
}


QString StdTable::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if(isSelected(rowBase, rowOffset) == true)
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));

    //return "c " + QString::number(col) + " r " + QString::number(rowBase + rowOffset);
    return mData->at(col)->at(rowBase + rowOffset);
}

void StdTable::mouseMoveEvent(QMouseEvent* event)
{
    bool wAccept = true;

    if(mGuiState == StdTable::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if((transY(event->y()) >= 0) && (transY(event->y()) <= this->getTableHeigth()))
        {
            int wRowIndex = getTableOffset() + getIndexOffsetFromY(transY(event->y()));

            if(wRowIndex < getRowCount())
            {
                if(mIsMultiSelctionAllowed == true)
                    expandSelectionUpTo(wRowIndex);
                else
                    setSingleSelection(wRowIndex);

                repaint();

                wAccept = false;
            }
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseMoveEvent(event);
}


void StdTable::mousePressEvent(QMouseEvent* event)
{
    bool wAccept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(event->y() > getHeaderHeight())
            {
                int wRowIndex = getTableOffset() + getIndexOffsetFromY(transY(event->y()));

                if(wRowIndex < getRowCount())
                {
                    if(mIsMultiSelctionAllowed && (event->modifiers() & Qt::ShiftModifier))
                        expandSelectionUpTo(wRowIndex);
                    else
                        setSingleSelection(wRowIndex);

                    mGuiState = StdTable::MultiRowsSelectionState;

                    repaint();

                    wAccept = true;
                }
            }
        }
    }

    if(wAccept == false)
        AbstractTableView::mousePressEvent(event);
}

void StdTable::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
        emit doubleClickedSignal();
    AbstractTableView::mouseDoubleClickEvent(event);
}

void StdTable::mouseReleaseEvent(QMouseEvent* event)
{
    bool wAccept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == StdTable::MultiRowsSelectionState)
        {
            mGuiState = StdTable::NoState;

            repaint();

            wAccept = false;
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseReleaseEvent(event);
}


void StdTable::keyPressEvent(QKeyEvent* event)
{
    emit keyPressedSignal(event);
    int key = event->key();

    if(key == Qt::Key_Up || key == Qt::Key_Down)
    {
        int_t wBotIndex = getTableOffset();
        int_t wTopIndex = wBotIndex + getNbrOfLineToPrint() - 1;

        if(key == Qt::Key_Up)
            selectPrevious();
        else
            selectNext();

        if(getInitialSelection() < wBotIndex)
        {
            setTableOffset(getInitialSelection());
        }
        else if(getInitialSelection() >= wTopIndex)
        {
            setTableOffset(getInitialSelection() - getNbrOfLineToPrint() + 2);
        }

        repaint();
    }
    else
    {
        AbstractTableView::keyPressEvent(event);
    }
}


void StdTable::enableMultiSelection(bool enabled)
{
    mIsMultiSelctionAllowed = enabled;
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void StdTable::expandSelectionUpTo(int to)
{
    if(to < mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = to;
        mSelection.toIndex = mSelection.firstSelectedIndex;
    }
    else if(to > mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = mSelection.firstSelectedIndex;
        mSelection.toIndex = to;
    }
    else if(to == mSelection.firstSelectedIndex)
    {
        setSingleSelection(to);
    }
}


void StdTable::setSingleSelection(int index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
    emit selectionChangedSignal(index);
}


int StdTable::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}


void StdTable::selectNext()
{
    int wNext = getInitialSelection() + 1;

    // Bounding
    wNext = wNext > getRowCount() - 1 ? getRowCount() - 1 : wNext;
    wNext = wNext < 0  ? 0 : wNext;

    setSingleSelection(wNext);
}


void StdTable::selectPrevious()
{
    int wNext = getInitialSelection() - 1;

    // Bounding
    wNext = wNext > getRowCount() - 1 ? getRowCount() - 1 : wNext;
    wNext = wNext < 0  ? 0 : wNext;

    setSingleSelection(wNext);
}


bool StdTable::isSelected(int base, int offset)
{
    int wIndex = base + offset;

    if(wIndex >= mSelection.fromIndex && wIndex <= mSelection.toIndex)
        return true;
    else
        return false;
}



/************************************************************************************
                                Data Management
************************************************************************************/
void StdTable::addColumnAt(int width, QString title, bool isClickable, QString copyTitle)
{
    AbstractTableView::addColumnAt(width, title, isClickable);

    mData->append(new QList<QString>());

    for(int wI = 0; wI < (getRowCount() - mData->last()->size()); wI++)
    {
        mData->last()->append(QString(""));
    }

    //Append copy title
    if(!copyTitle.length())
        mCopyTitles.append(title);
    else
        mCopyTitles.append(copyTitle);
}



void StdTable::setRowCount(int count)
{
    int wI, wJ;

    for(wJ = 0; wJ < getColumnCount(); wJ++)
    {
        int  wRowToAddOrRemove = count - mData->at(wJ)->size();

        for(wI = 0; wI < qAbs(wRowToAddOrRemove); wI++)
        {
            if(wRowToAddOrRemove > 0)
                mData->at(wJ)->append(QString(""));
            else
                mData->at(wJ)->removeLast();
        }
    }

    AbstractTableView::setRowCount(count);
}

void StdTable::deleteAllColumns()
{
    setRowCount(0);
    AbstractTableView::deleteAllColumns();
    mCopyTitles.clear();
}

void StdTable::setCellContent(int r, int c, QString s)
{
    if(isValidIndex(r, c) == true)
        mData->at(c)->replace(r, s);
}


QString StdTable::getCellContent(int r, int c)
{
    if(isValidIndex(r, c) == true)
        return QString(mData->at(c)->at(r));
    else
        return QString("");
}


bool StdTable::isValidIndex(int r, int c)
{
    if(mData->isEmpty() == false && c >= 0 && c < mData->size())
    {
        if(mData->at(c)->isEmpty() == false && r >= 0 && r < mData->at(c)->size())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void StdTable::copyLineSlot()
{
    int colCount = getColumnCount();
    QString finalText = "";
    if(colCount == 1)
        finalText = getCellContent(getInitialSelection(), 0);
    else
    {
        for(int i=0; i<colCount; i++)
        {
            QString cellContent = getCellContent(getInitialSelection(), i);
            if(!cellContent.length()) //skip empty cells
                continue;
            QString title = mCopyTitles.at(i);
            if(title.length())
                finalText += title + "=";
            finalText += cellContent;
            finalText += "\r\n";
        }
    }
    Bridge::CopyToClipboard(finalText.toUtf8().constData());
}

void StdTable::copyTableSlot()
{
    int colCount = getColumnCount();
    int rowCount = getRowCount();
    QString finalText = "";
    if(colCount == 1)
    {
        for(int i=0; i<rowCount; i++)
        {
            QString cellContent = getCellContent(i, 0);
            if(!cellContent.length()) //skip empty cells
                continue;
            finalText += cellContent + "\r\n";
        }
    }
    else
    {
        int charwidth=getCharWidth();
        for(int i=0; i<colCount; i++)
        {
            if(i)
                finalText += " ";
            int colWidth = getColumnWidth(i)/charwidth;
            if(colWidth)
                finalText += getColTitle(i).leftJustified(colWidth, QChar(' '), true);
            else
                finalText += getColTitle(i);
        }
        finalText += "\r\n";
        for(int i=0; i<rowCount; i++)
        {
            QString finalRowText = "";
            for(int j=0; j<colCount; j++)
            {
                if(j)
                    finalRowText += " ";
                QString cellContent = getCellContent(i, j);
                int colWidth = getColumnWidth(j)/charwidth;
                if(colWidth)
                    finalRowText += cellContent.leftJustified(colWidth, QChar(' '), true);
                else
                    finalRowText += cellContent;
            }
            finalText += finalRowText + "\r\n";
        }
    }
    Bridge::CopyToClipboard(finalText.toUtf8().constData());
}

void StdTable::copyEntrySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    int col=action->objectName().toInt();
    Bridge::CopyToClipboard(getCellContent(getInitialSelection(), col).toUtf8().constData());
}

void StdTable::setupCopyMenu(QMenu* copyMenu)
{
    if(!getColumnCount())
        return;
    //Copy->Whole Line
    QAction* mCopyLine = new QAction("Whole &Line", this);
    connect(mCopyLine, SIGNAL(triggered()), this, SLOT(copyLineSlot()));
    copyMenu->addAction(mCopyLine);
    //Copy->Whole Table
    QAction* mCopyTable = new QAction("Whole &Table", this);
    connect(mCopyTable, SIGNAL(triggered()), this, SLOT(copyTableSlot()));
    copyMenu->addAction(mCopyTable);
    //Copy->Separatoe
    copyMenu->addSeparator();
    //Copy->ColName
    for(int i=0; i<getColumnCount(); i++)
    {
        if(!getCellContent(getInitialSelection(), i).length()) //skip empty cells
            continue;
        QString title=mCopyTitles.at(i);
        if(!title.length()) //skip empty copy titles
            continue;
        QAction* mCopyAction = new QAction(title, this);
        mCopyAction->setObjectName(QString::number(i));
        connect(mCopyAction, SIGNAL(triggered()), this, SLOT(copyEntrySlot()));
        copyMenu->addAction(mCopyAction);
    }
}

void StdTable::setCopyMenuOnly(bool bSet, bool bDebugOnly)
{
    mCopyMenuOnly = bSet;
    mCopyMenuDebugOnly = bDebugOnly;
}

void StdTable::contextMenuRequestedSlot(const QPoint &pos)
{
    if(!mCopyMenuOnly)
    {
        emit contextMenuSignal(pos);
        return;
    }
    if(mCopyMenuDebugOnly && !DbgIsDebugging())
        return;
    QMenu* wMenu = new QMenu(this);
    QMenu wCopyMenu("&Copy", this);
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
        wMenu->exec(mapToGlobal(pos));
    }
}
