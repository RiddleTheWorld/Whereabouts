// Checks the layout view's viewport arithmetic: that the scrollbars actually
// move the canvas, that a click in list mode lands on the row under the
// pointer, and that the list highlights a hovered row. Development helper only;
// lives in tools/ and never ships.
//
// All three were broken together because paintEvent draws from m_origin while
// the scrollbars, the hit test and the hover handler each disagreed about how
// that origin is built.
#include <QApplication>
#include <QImage>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTextStream>

#include "layoutview.h"
#include "sessionmodel.h"

static QTextStream out(stdout);

static int failures = 0;

static void check(bool ok, const QString &what)
{
    out << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok)
        ++failures;
}

// A session with enough items that the canvas is larger than the viewport in
// both modes, so both scrollbars have something to scroll.
static Session makeSession(int columns, int rows)
{
    Session session;
    session.id = QStringLiteral("scrollcheck");
    session.name = QStringLiteral("scrollcheck");
    session.rootPath = QStringLiteral("C:/scrollcheck");
    session.capturedAt = QDateTime::currentDateTime();
    session.screen = QSize(1920, 1080);
    session.cellSize = QSize(75, 98);

    int index = 0;
    for (int column = 0; column < columns; ++column) {
        for (int row = 0; row < rows; ++row) {
            Entry entry;
            entry.name = QStringLiteral("item%1").arg(index, 3, 10, QLatin1Char('0'));
            entry.relativePath = entry.name;
            entry.kind = Kind::Document;
            entry.suffix = QStringLiteral("txt");
            entry.size = 1024;
            entry.modified = QDateTime::currentDateTime();
            entry.cell = QPoint(column, row);
            entry.pos = QPoint(column * 75, row * 98);
            session.entries.append(entry);
            ++index;
        }
    }
    return session;
}

static void sendMove(LayoutView *view, const QPointF &pos)
{
    QMouseEvent move(QEvent::MouseMove, pos, view->viewport()->mapToGlobal(pos),
                     Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view->viewport(), &move);
}

static void sendClick(LayoutView *view, const QPointF &pos)
{
    QMouseEvent press(QEvent::MouseButtonPress, pos, view->viewport()->mapToGlobal(pos),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view->viewport(), &press);
    QMouseEvent release(QEvent::MouseButtonRelease, pos, view->viewport()->mapToGlobal(pos),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view->viewport(), &release);
}

// Clicks the centre of a few painted tiles and checks each click selects the
// tile that was drawn there. Scrolled to `scrollTo` first, because the point of
// the exercise is that the hit test and the painter agree at any offset.
static void clickTileCentres(LayoutView *view, QApplication *app,
                             const QString &what, int scrollTo)
{
    view->verticalScrollBar()->setValue(scrollTo);
    app->processEvents();

    const QHash<QString, QRectF> rects = view->tileRects();
    const QRect inside = view->viewport()->rect().adjusted(2, 2, -2, -2);

    int tested = 0;
    for (auto it = rects.constBegin(); it != rects.constEnd() && tested < 3; ++it) {
        const QPointF centre = it.value().center();
        if (!inside.contains(centre.toPoint()))
            continue;
        sendClick(view, centre);
        check(view->selectedPath() == it.key(),
              QStringLiteral("%1: clicking the tile drawn at %2,%3 selects %4 (got %5)")
                  .arg(what)
                  .arg(qRound(centre.x()))
                  .arg(qRound(centre.y()))
                  .arg(it.key(),
                       view->selectedPath().isEmpty() ? QStringLiteral("nothing")
                                                      : view->selectedPath()));
        ++tested;
    }
    check(tested > 0, QStringLiteral("%1: at least one tile is on screen").arg(what));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("CairnScrollCheck"));

    const Session session = makeSession(12, 9);   // 108 items

    LayoutView view;
    view.resize(400, 300);
    view.setSession(session);
    view.show();
    app.processEvents();

    out << "Map mode\n";
    check(view.verticalScrollBar()->maximum() > 0,
          QStringLiteral("the map is taller than the viewport (vertical range %1)")
              .arg(view.verticalScrollBar()->maximum()));

    const QImage mapTop = view.viewport()->grab().toImage();
    view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());
    app.processEvents();
    const QImage mapBottom = view.viewport()->grab().toImage();
    check(mapTop != mapBottom,
          QStringLiteral("scrolling the map repaints somewhere else"));

    // A click has to select whatever is painted under it, at any scroll offset:
    // tileRects() reports where each tile was drawn, so clicking the centre of
    // one must select that very tile.
    clickTileCentres(&view, &app, QStringLiteral("map, unscrolled"), 0);
    clickTileCentres(&view, &app, QStringLiteral("map, scrolled to the bottom"),
                     view.verticalScrollBar()->maximum());

    out << "\nList mode\n";
    view.setMode(LayoutView::Mode::List);
    app.processEvents();

    // Rows are kListRowHeight tall and start one margin down. Whatever order the
    // list is sorted in, clicking row i has to select the i-th visible path.
    const QList<QString> order = view.visiblePaths();
    check(order.size() == session.entries.size(),
          QStringLiteral("every item gets a row"));
    check(view.verticalScrollBar()->value() == 0,
          QStringLiteral("switching modes starts at the top of the list"));
    for (int row : {0, 1, 2, 5}) {
        const qreal y = 18.0 + row * 30.0 + 14.0;
        sendClick(&view, QPointF(120, y));
        const QString expected = order.value(row);
        check(view.selectedPath() == expected,
              QStringLiteral("clicking row %1 selects %2 (got %3)")
                  .arg(row)
                  .arg(expected,
                       view.selectedPath().isEmpty() ? QStringLiteral("nothing")
                                                     : view.selectedPath()));
    }

    check(view.verticalScrollBar()->maximum() > 0,
          QStringLiteral("the list is longer than the viewport (vertical range %1)")
              .arg(view.verticalScrollBar()->maximum()));
    const QImage listTop = view.viewport()->grab().toImage();
    view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());
    app.processEvents();
    const QImage listBottom = view.viewport()->grab().toImage();
    check(listTop != listBottom,
          QStringLiteral("scrolling the list repaints somewhere else"));

    // Hovering a row has to light it up. The pointer is parked off the rows
    // first so the comparison is against an unhovered list.
    view.verticalScrollBar()->setValue(0);
    app.processEvents();
    sendMove(&view, QPointF(120, 4));
    app.processEvents();
    const QImage unhovered = view.viewport()->grab().toImage();
    sendMove(&view, QPointF(120, 18.0 + 3 * 30.0 + 14.0));
    app.processEvents();
    const QImage hovered = view.viewport()->grab().toImage();
    check(unhovered != hovered,
          QStringLiteral("a hovered list row is highlighted"));

    out << "\n";
    out << (failures == 0 ? "SCROLLCHECK PASSED\n" : "SCROLLCHECK FAILED\n");
    out.flush();
    return failures == 0 ? 0 : 1;
}
