#pragma once

#include "sessionmodel.h"

#include <QAbstractScrollArea>
#include <QColor>
#include <QHash>
#include <QPixmap>
#include <QPointF>
#include <QString>

// The snapshot as a map: a dark canvas with one labelled tile per item, placed
// where the item actually sits.
//
// Positions read from Explorer are used verbatim, so a folder really does look
// the way it did. Everything else is laid out on a grid by the capture step.
// The view is an abstract canvas, not a screenshot: the whole layout is scaled
// to fit, so a 2560px-wide desktop is readable in a small window.
class LayoutView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    enum class Mode { Map, List };

    // List ordering. Map mode is always positional, so this only applies to the
    // list; the sort is kept across a mode switch so it is not lost on a toggle.
    enum class Sort { Name, Size, Modified, Kind };

    explicit LayoutView(QWidget *parent = nullptr);

    void setSession(const Session &session);
    void setSession(const Session &session, bool resetView);

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    void setKindFilter(const QHash<int, bool> &enabledByKind);
    void setSearch(const QString &text);

    void setSort(Sort sort, bool descending = false);
    Sort sort() const { return m_sort; }

    QString selectedPath() const { return m_selectedPath; }
    QList<QString> visiblePaths() const;

    // Where each visible tile is drawn, in viewport pixels. Exists so tests can
    // assert that tiles really land in different places instead of all
    // collapsing onto one spot.
    QHash<QString, QRectF> tileRects() const;

    // Scales the canvas to fit the viewport. Called automatically on resize
    // unless the user has taken over with the zoom controls.
    void fitToView();
    void setZoom(qreal zoom);
    qreal zoom() const { return m_zoom; }

    // Moves the selection by one tile in the given direction, wrapping at the
    // edges, and reports the newly selected path. Returns false when there is
    // nothing to move through. Arrow keys drive this.
    bool moveSelection(int dx, int dy);

    // Clears the selection. Returns true if something was selected.
    bool clearSelection();

    // Activates the current selection (the same thing a double-click does).
    bool activateSelection();

signals:
    void entryActivated(const QString &relativePath);
    void entrySelected(const QString &relativePath);
    void viewChanged();   // selection, zoom or filtering changed anything visible

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;

public:
    // Scrolls the selected tile into view, if any. Public so the window can call
    // it after programmatic selection.
    void ensureSelectionVisible();

private:
    // Drops the selection when a filter or a search hides the selected
    // item, so the window can fall back to the session summary.
    void dropSelectionIfHidden();
    struct Tile {
        QString path;
        QRectF rect;            // canvas coordinates
        QString name;
        Kind kind = Kind::Other;
        QString suffix;
        qint64 size = 0;
        bool viaShell = false;
        QString target;
        int children = -1;
    };

    void rebuild();
    void rebuildMap();
    void rebuildList();
    void updateScrollRanges();
    // Recomputes m_origin from the scrollbars, the zoom and the pan. Separate
    // from updateScrollRanges() so a scroll can move the canvas without
    // touching the ranges that caused the scroll.
    void updateOrigin();
    // Sends both bars back to the start, for when the content underneath them
    // is replaced by something with an unrelated coordinate space.
    void resetScroll();
    void updateHover(const QPoint &viewportPos);

    // The factor between canvas coordinates and viewport pixels. The map is
    // zoomed; the list is always drawn at 1:1, so every piece of viewport
    // arithmetic has to ask rather than assume m_zoom.
    qreal canvasScale() const { return m_mode == Mode::Map ? m_zoom : 1.0; }

    int hitTest(const QPointF &viewportPos) const;

    bool isVisible(const Entry &entry) const;
    QString elided(const QString &text, const QFont &font, qreal width) const;

    Session m_session;
    Mode m_mode = Mode::Map;
    QVector<Tile> m_tiles;
    QHash<int, bool> m_kindFilter;
    QString m_search;
    Sort m_sort = Sort::Name;
    bool m_sortDescending = false;
    QString m_selectedPath;
    QString m_hoveredPath;

    qreal m_zoom = 1.0;
    QSizeF m_canvasSize;        // natural size of the laid-out content
    QPointF m_origin;           // canvas offset inside the viewport
    QPointF m_panOffset;        // user pan, added on top of the centring
    QPointF m_panStartOffset;   // pan in effect when the current drag began
    QPoint m_dragStart;
    bool m_dragging = false;
    bool m_userZoomed = false;
    QString m_emptyMessage;
};
