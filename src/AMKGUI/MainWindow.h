#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QProgressBar>
#include <QTimer>
#include "SongList.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void parseListFiles();
    void onRunClicked();
    void onAddGlobalClicked();
    void onChangeGlobalClicked();
    void onAddLocalClicked();
    void onChangeLocalClicked();
    void onRemoveLocalClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onPorterModeToggled(bool checked);
    void onPlaySPCClicked();
    void onSelectROMClicked();
    void onSelectListFolderClicked();
    void updateButtonStates();

private:
    void setupUI();
    void refreshListBoxes();
    QString toRelativePath(const QString& path);
    void runAMK();
    void playSPC(int localSongIndex);
    void showResultDialog(const QString& output, bool success);

    // Song lists
    SongList globalSongs;
    SongList localSongs;

    // Paths
    QString romPath;
    QString listFolderPath;

    // UI elements
    QListWidget *globalListWidget;
    QListWidget *localListWidget;
    QCheckBox *porterModeCheckBox;
    QCheckBox *verboseCheckBox;
    QCheckBox *playSPCCheckBox;
    QLineEdit *romPathEdit;
    QLineEdit *listFolderEdit;
    QPushButton *runButton;
    QPushButton *selectROMButton;
    QPushButton *selectListButton;
    QPushButton *addGlobalButton;
    QPushButton *changeGlobalButton;
    QPushButton *addLocalButton;
    QPushButton *changeLocalButton;
    QPushButton *removeLocalButton;
    QPushButton *moveUpButton;
    QPushButton *moveDownButton;
    QPushButton *playSPCButton;
    QTextEdit *outputTextEdit;
    QProgressBar *progressBar;
    QTimer *buttonUpdateTimer;

    // Advanced options
    QGroupBox *advancedGroup;
    QCheckBox *aggressiveCheckBox;
    QCheckBox *bankOptOffCheckBox;
    QCheckBox *echoCheckOffCheckBox;
    QCheckBox *dupCheckOffCheckBox;
    QCheckBox *sampleOptOffCheckBox;
    QCheckBox *hexValidOffCheckBox;
    QCheckBox *sa1OffCheckBox;
};

#endif // MAINWINDOW_H