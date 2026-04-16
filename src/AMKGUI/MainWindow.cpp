#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QApplication>
#include <QKeySequence>
#include <QShortcut>

// Include AMK headers
#include "defines.h"
#include "AddmusicK.h"
#include "SPCEnvironment.h"
#include "ROMEnvironment.h"

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    parseListFiles();

    // Set up keyboard shortcut
    QShortcut *runShortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
    connect(runShortcut, &QShortcut::activated, this, &MainWindow::onRunClicked);

    // Set up button update timer
    buttonUpdateTimer = new QTimer(this);
    connect(buttonUpdateTimer, &QTimer::timeout, this, &MainWindow::updateButtonStates);
    buttonUpdateTimer->start(100); // Update every 100ms
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("AMKGUI v1.0.8");
    setMinimumSize(800, 600);

    // Create central widget
    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Top section: ROM and List folder selection
    QGroupBox *fileGroup = new QGroupBox("Files");
    QFormLayout *fileLayout = new QFormLayout(fileGroup);

    romPathEdit = new QLineEdit;
    selectROMButton = new QPushButton("Browse...");
    connect(selectROMButton, &QPushButton::clicked, this, &MainWindow::onSelectROMClicked);

    QHBoxLayout *romLayout = new QHBoxLayout;
    romLayout->addWidget(romPathEdit);
    romLayout->addWidget(selectROMButton);
    fileLayout->addRow("ROM File:", romLayout);

    listFolderEdit = new QLineEdit;
    selectListButton = new QPushButton("Browse...");
    connect(selectListButton, &QPushButton::clicked, this, &MainWindow::onSelectListFolderClicked);

    QHBoxLayout *listLayout = new QHBoxLayout;
    listLayout->addWidget(listFolderEdit);
    listLayout->addWidget(selectListButton);
    fileLayout->addRow("List Folder:", listLayout);

    mainLayout->addWidget(fileGroup);

    // Mode selection
    QGroupBox *modeGroup = new QGroupBox("Mode");
    QVBoxLayout *modeLayout = new QVBoxLayout(modeGroup);

    porterModeCheckBox = new QCheckBox("Porter Mode (Compile individual songs)");
    connect(porterModeCheckBox, &QCheckBox::toggled, this, &MainWindow::onPorterModeToggled);
    modeLayout->addWidget(porterModeCheckBox);

    verboseCheckBox = new QCheckBox("Verbose output");
    modeLayout->addWidget(verboseCheckBox);

    playSPCCheckBox = new QCheckBox("Play SPC after compilation");
    modeLayout->addWidget(playSPCCheckBox);

    mainLayout->addWidget(modeGroup);

    // Song lists section
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Global songs
    QGroupBox *globalGroup = new QGroupBox("Global Songs");
    QVBoxLayout *globalLayout = new QVBoxLayout(globalGroup);

    globalListWidget = new QListWidget;
    globalLayout->addWidget(globalListWidget);

    QHBoxLayout *globalButtonLayout = new QHBoxLayout;
    addGlobalButton = new QPushButton("Add");
    connect(addGlobalButton, &QPushButton::clicked, this, &MainWindow::onAddGlobalClicked);
    globalButtonLayout->addWidget(addGlobalButton);

    changeGlobalButton = new QPushButton("Change");
    connect(changeGlobalButton, &QPushButton::clicked, this, &MainWindow::onChangeGlobalClicked);
    globalButtonLayout->addWidget(changeGlobalButton);

    globalLayout->addLayout(globalButtonLayout);
    splitter->addWidget(globalGroup);

    // Local songs
    QGroupBox *localGroup = new QGroupBox("Local Songs");
    QVBoxLayout *localLayout = new QVBoxLayout(localGroup);

    localListWidget = new QListWidget;
    localLayout->addWidget(localListWidget);

    QHBoxLayout *localButtonLayout = new QHBoxLayout;
    addLocalButton = new QPushButton("Add");
    connect(addLocalButton, &QPushButton::clicked, this, &MainWindow::onAddLocalClicked);
    localButtonLayout->addWidget(addLocalButton);

    changeLocalButton = new QPushButton("Change");
    connect(changeLocalButton, &QPushButton::clicked, this, &MainWindow::onChangeLocalClicked);
    localButtonLayout->addWidget(changeLocalButton);

    removeLocalButton = new QPushButton("Remove");
    connect(removeLocalButton, &QPushButton::clicked, this, &MainWindow::onRemoveLocalClicked);
    localButtonLayout->addWidget(removeLocalButton);

    moveUpButton = new QPushButton("↑");
    connect(moveUpButton, &QPushButton::clicked, this, &MainWindow::onMoveUpClicked);
    localButtonLayout->addWidget(moveUpButton);

    moveDownButton = new QPushButton("↓");
    connect(moveDownButton, &QPushButton::clicked, this, &MainWindow::onMoveDownClicked);
    localButtonLayout->addWidget(moveDownButton);

    playSPCButton = new QPushButton("Play SPC");
    connect(playSPCButton, &QPushButton::clicked, this, &MainWindow::onPlaySPCClicked);
    localButtonLayout->addWidget(playSPCButton);

    localLayout->addLayout(localButtonLayout);
    splitter->addWidget(localGroup);

    mainLayout->addWidget(splitter);

    // Advanced options
    advancedGroup = new QGroupBox("Advanced Options");
    advancedGroup->setCheckable(true);
    advancedGroup->setChecked(false);
    QGridLayout *advancedLayout = new QGridLayout(advancedGroup);

    aggressiveCheckBox = new QCheckBox("Aggressive ROM space finding");
    advancedLayout->addWidget(aggressiveCheckBox, 0, 0);

    bankOptOffCheckBox = new QCheckBox("Turn off bank optimizations");
    advancedLayout->addWidget(bankOptOffCheckBox, 0, 1);

    echoCheckOffCheckBox = new QCheckBox("Turn off echo buffer bounds checking");
    advancedLayout->addWidget(echoCheckOffCheckBox, 1, 0);

    dupCheckOffCheckBox = new QCheckBox("Turn off sample duplicate checking");
    advancedLayout->addWidget(dupCheckOffCheckBox, 1, 1);

    sampleOptOffCheckBox = new QCheckBox("Turn off sample usage optimizations");
    advancedLayout->addWidget(sampleOptOffCheckBox, 2, 0);

    hexValidOffCheckBox = new QCheckBox("Turn off hex command validation");
    advancedLayout->addWidget(hexValidOffCheckBox, 2, 1);

    sa1OffCheckBox = new QCheckBox("Turn off SA1 addressing");
    advancedLayout->addWidget(sa1OffCheckBox, 3, 0);

    mainLayout->addWidget(advancedGroup);

    // Output section
    QGroupBox *outputGroup = new QGroupBox("Output");
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);

    outputTextEdit = new QTextEdit;
    outputTextEdit->setReadOnly(true);
    outputLayout->addWidget(outputTextEdit);

    progressBar = new QProgressBar;
    progressBar->setVisible(false);
    outputLayout->addWidget(progressBar);

    mainLayout->addWidget(outputGroup);

    // Run button
    runButton = new QPushButton("Run AMK");
    runButton->setStyleSheet("QPushButton { font-weight: bold; padding: 10px; }");
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunClicked);
    mainLayout->addWidget(runButton);

    // Set initial button states
    updateButtonStates();
}

void MainWindow::parseListFiles()
{
    try {
        QString filePath;
        if (!listFolderPath.isEmpty()) {
            filePath = listFolderPath + "/Addmusic_list.txt";
        } else if (QFile::exists("Addmusic_list.txt")) {
            filePath = "Addmusic_list.txt";
        } else {
            // Ask user to select list file
            QString selectedFile = QFileDialog::getOpenFileName(this, "Select Addmusic_list.txt", "", "Text files (*.txt)");
            if (selectedFile.isEmpty()) {
                QMessageBox::information(this, "Quitting", "No list file selected. Quitting.");
                close();
                return;
            }
            filePath = selectedFile;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            throw std::runtime_error("Cannot open list file");
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // Update list folder path
        QFileInfo fi(filePath);
        listFolderPath = fi.absolutePath();
        listFolderEdit->setText(listFolderPath);

        globalSongs = SongList::parseSongs(content, "Globals");
        localSongs = SongList::parseSongs(content, "Locals");

    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", QString("Error parsing list file: %1").arg(ex.what()));
        close();
        return;
    }

    refreshListBoxes();
}

void MainWindow::refreshListBoxes()
{
    globalListWidget->clear();
    for (size_t i = 1; i < globalSongs.fileNames.size(); ++i) {
        if (!globalSongs.fileNames[i].isEmpty()) {
            globalListWidget->addItem(QString("%1:\t%2").arg(i, 2, 16, QChar('0')).arg(globalSongs.fileNames[i]));
        }
    }

    localListWidget->clear();
    for (size_t i = globalSongs.fileNames.size(); i < localSongs.fileNames.size(); ++i) {
        if (!localSongs.fileNames[i].isEmpty()) {
            localListWidget->addItem(QString("%1:\t%2").arg(i, 2, 16, QChar('0')).arg(localSongs.fileNames[i]));
        }
    }
}

QString MainWindow::toRelativePath(const QString& path)
{
    if (QFileInfo(listFolderPath).isAbsolute()) {
        return QDir(listFolderPath + "/music").relativeFilePath(path);
    } else {
        return "music/" + path;
    }
}

void MainWindow::onRunClicked()
{
    runAMK();
}

void MainWindow::runAMK()
{
    if (porterModeCheckBox->isChecked() && localListWidget->currentRow() == -1) {
        QMessageBox::warning(this, "No song selected", "Please select a song to compile in porter mode.");
        return;
    }

    try {
        // Save the list file
        QString writeStr = globalSongs.toString() + localSongs.toString();
        QFile file(listFolderPath + "/Addmusic_list.txt");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("Cannot write to list file");
        }
        QTextStream out(&file);
        out << writeStr;
        file.close();

        // Prepare options
        AddMusic::EnvironmentOptions spc_options;
        spc_options.verbose = verboseCheckBox->isChecked();
        spc_options.aggressive = aggressiveCheckBox->isChecked();
        spc_options.bankOptimizations = !bankOptOffCheckBox->isChecked();
        spc_options.checkEcho = !echoCheckOffCheckBox->isChecked();
        spc_options.dupCheck = !dupCheckOffCheckBox->isChecked();
        spc_options.optimizeSampleUsage = !sampleOptOffCheckBox->isChecked();
        spc_options.validateHex = !hexValidOffCheckBox->isChecked();
        spc_options.allowSA1 = !sa1OffCheckBox->isChecked();

        outputTextEdit->clear();
        progressBar->setVisible(true);
        progressBar->setRange(0, 0); // Indeterminate progress
        runButton->setEnabled(false);
        QApplication::processEvents();

        if (!porterModeCheckBox->isChecked()) {
            // ROM patching mode
            if (romPath.isEmpty()) {
                QString selectedROM = QFileDialog::getOpenFileName(this, "Select SMW ROM", "", "SNES ROMs (*.smc *.sfc)");
                if (selectedROM.isEmpty()) return;
                romPath = selectedROM;
                romPathEdit->setText(romPath);
            }

            fs::path rom_location = fs::path(romPath.toStdString());
            fs::path list_folder = fs::path(listFolderPath.toStdString());
            fs::path output = rom_location;

            if (!fs::exists(rom_location)) {
                QMessageBox::critical(this, "Error", "ROM file does not exist.");
                return;
            }

            if (fs::equivalent(rom_location, output)) {
                QMessageBox::StandardButton reply = QMessageBox::question(this,
                    "Overwrite ROM", "The original ROM file will be overwritten. Is this ok?",
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::No) return;
            }

            AddMusic::ROMEnvironment rom_env(rom_location, list_folder, spc_options);
            rom_env.patchROM(output);

            outputTextEdit->append("ROM successfully patched and saved to: " + QString::fromStdString(output.string()));
            showResultDialog("ROM successfully patched!", true);

        } else {
            // Song compilation mode
            int selectedIndex = localListWidget->currentRow();
            if (selectedIndex == -1) return;

            size_t songIndex = selectedIndex + globalSongs.fileNames.size();
            QString mmlFile = localSongs.fileNames[songIndex];

            fs::path list_folder = fs::path(listFolderPath.toStdString());
            fs::path output_dir = list_folder / "SPCs";

            std::vector<fs::path> mml_paths = {list_folder / "music" / fs::path(mmlFile.toStdString())};

            AddMusic::SPCEnvironment spc_env(list_folder, spc_options);
            spc_env.generateSPCFiles(mml_paths, output_dir);

            outputTextEdit->append("Song compiled successfully!");
            showResultDialog("Song compiled successfully!", true);

            if (playSPCCheckBox->isChecked()) {
                playSPC(songIndex);
            }
        }

    } catch (const std::exception& ex) {
        outputTextEdit->append("Error: " + QString(ex.what()));
        showResultDialog("Error: " + QString(ex.what()), false);
    }

    progressBar->setVisible(false);
    runButton->setEnabled(true);
}

void MainWindow::playSPC(int localSongIndex)
{
    if (!playSPCCheckBox->isChecked()) return;

    QString spcFilePath = localSongs.fileNames[localSongIndex];
    if (spcFilePath.lastIndexOf('/') >= 0) {
        spcFilePath = spcFilePath.mid(spcFilePath.lastIndexOf('/') + 1);
    }
    if (spcFilePath.lastIndexOf('.') >= 0) {
        spcFilePath = spcFilePath.left(spcFilePath.lastIndexOf('.')) + ".spc";
    }

    QString fullPath = listFolderPath + "/SPCs/" + spcFilePath;

    if (!QFile::exists(fullPath)) {
        QMessageBox::warning(this, "SPC not found", "The SPC file has not been generated yet. Please run the compilation first.");
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
}

void MainWindow::showResultDialog(const QString& output, bool success)
{
    QMessageBox msgBox;
    msgBox.setText(output);
    msgBox.setIcon(success ? QMessageBox::Information : QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::onAddGlobalClicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select MML file", listFolderPath + "/music", "MML files (*.txt)");
    if (!file.isEmpty()) {
        int index = globalListWidget->currentRow();
        if (index == -1) index = globalListWidget->count();
        // For simplicity, add to next available slot
        for (size_t i = 1; i < globalSongs.fileNames.size(); ++i) {
            if (globalSongs.fileNames[i].isEmpty()) {
                globalSongs.fileNames[i] = toRelativePath(file);
                break;
            }
        }
        refreshListBoxes();
    }
}

void MainWindow::onChangeGlobalClicked()
{
    if (globalListWidget->currentRow() == -1) return;
    QString file = QFileDialog::getOpenFileName(this, "Select MML file", listFolderPath + "/music", "MML files (*.txt)");
    if (!file.isEmpty()) {
        int index = globalListWidget->currentRow();
        // Find the corresponding song index
        int songIndex = 1;
        for (int i = 0; i <= index; ++i) {
            while (songIndex < (int)globalSongs.fileNames.size() && globalSongs.fileNames[songIndex].isEmpty()) ++songIndex;
            if (i == index) break;
            ++songIndex;
        }
        globalSongs.fileNames[songIndex] = toRelativePath(file);
        refreshListBoxes();
    }
}

void MainWindow::onAddLocalClicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select MML file", listFolderPath + "/music", "MML files (*.txt)");
    if (!file.isEmpty()) {
        if (localListWidget->count() == 0) {
            localSongs.fileNames.resize(globalSongs.fileNames.size(), "");
            localSongs.fileNames.push_back(toRelativePath(file));
        } else {
            int index = localListWidget->currentRow();
            if (index == -1) index = localListWidget->count() - 1;
            size_t songIndex = index + globalSongs.fileNames.size() + 1;
            if (songIndex >= localSongs.fileNames.size()) {
                localSongs.fileNames.resize(songIndex + 1, "");
            }
            localSongs.fileNames[songIndex] = toRelativePath(file);
        }
        refreshListBoxes();
        localListWidget->setCurrentRow(localListWidget->count() - 1);
    }
}

void MainWindow::onChangeLocalClicked()
{
    if (localListWidget->currentRow() == -1) return;
    QString file = QFileDialog::getOpenFileName(this, "Select MML file", listFolderPath + "/music", "MML files (*.txt)");
    if (!file.isEmpty()) {
        int index = localListWidget->currentRow();
        size_t songIndex = index + globalSongs.fileNames.size();
        localSongs.fileNames[songIndex] = toRelativePath(file);
        refreshListBoxes();
    }
}

void MainWindow::onRemoveLocalClicked()
{
    if (localListWidget->currentRow() == -1) return;
    int index = localListWidget->currentRow();
    size_t songIndex = index + globalSongs.fileNames.size();
    localSongs.fileNames[songIndex] = "";
    refreshListBoxes();
}

void MainWindow::onMoveUpClicked()
{
    if (localListWidget->currentRow() <= 0) return;
    int index = localListWidget->currentRow();
    size_t songIndex = index + globalSongs.fileNames.size();
    std::swap(localSongs.fileNames[songIndex], localSongs.fileNames[songIndex - 1]);
    refreshListBoxes();
    localListWidget->setCurrentRow(index - 1);
}

void MainWindow::onMoveDownClicked()
{
    if (localListWidget->currentRow() == -1 || localListWidget->currentRow() >= localListWidget->count() - 1) return;
    int index = localListWidget->currentRow();
    size_t songIndex = index + globalSongs.fileNames.size();
    std::swap(localSongs.fileNames[songIndex], localSongs.fileNames[songIndex + 1]);
    refreshListBoxes();
    localListWidget->setCurrentRow(index + 1);
}

void MainWindow::onPorterModeToggled(bool checked)
{
    // Adjust UI for porter mode
    playSPCCheckBox->setVisible(checked);
    playSPCButton->setVisible(checked);
    romPathEdit->setEnabled(!checked);
    selectROMButton->setEnabled(!checked);
}

void MainWindow::onPlaySPCClicked()
{
    if (localListWidget->currentRow() == -1) return;
    size_t songIndex = localListWidget->currentRow() + globalSongs.fileNames.size();
    playSPC(songIndex);
}

void MainWindow::onSelectROMClicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select SMW ROM", "", "SNES ROMs (*.smc *.sfc)");
    if (!file.isEmpty()) {
        romPath = file;
        romPathEdit->setText(romPath);
    }
}

void MainWindow::onSelectListFolderClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select list folder");
    if (!dir.isEmpty()) {
        listFolderPath = dir;
        listFolderEdit->setText(listFolderPath);
        parseListFiles();
    }
}

void MainWindow::updateButtonStates()
{
    bool hasGlobalSelection = globalListWidget->currentRow() != -1;
    addGlobalButton->setEnabled(true);
    changeGlobalButton->setEnabled(hasGlobalSelection);

    bool hasLocalSelection = localListWidget->currentRow() != -1;
    addLocalButton->setEnabled(true);
    changeLocalButton->setEnabled(hasLocalSelection);
    removeLocalButton->setEnabled(hasLocalSelection);
    moveUpButton->setEnabled(hasLocalSelection && localListWidget->currentRow() > 0);
    moveDownButton->setEnabled(hasLocalSelection && localListWidget->currentRow() < localListWidget->count() - 1);
    playSPCButton->setEnabled(hasLocalSelection && porterModeCheckBox->isChecked());
}