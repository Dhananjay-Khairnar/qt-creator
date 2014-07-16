/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakeprojectplugin.h"
#include "cmakeprojectmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakerunconfiguration.h"
#include "cmakeeditorfactory.h"
#include "makestep.h"
#include "cmakeprojectconstants.h"
#include "cmakelocatorfilter.h"
#include "cmakefilecompletionassist.h"
#include "cmakehighlighter.h"

#include <coreplugin/featureprovider.h>
#include <coreplugin/mimedatabase.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/highlighterfactory.h>

#include <QtPlugin>
#include <QDebug>


using namespace CMakeProjectManager::Internal;

CMakeProjectPlugin::CMakeProjectPlugin()
{
}

CMakeProjectPlugin::~CMakeProjectPlugin()
{
}

bool CMakeProjectPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    if (!Core::MimeDatabase::addMimeTypes(QLatin1String(":cmakeproject/CMakeProjectManager.mimetypes.xml"), errorMessage))
        return false;
    CMakeSettingsPage *cmp = new CMakeSettingsPage();
    addAutoReleasedObject(cmp);
    CMakeManager *manager = new CMakeManager(cmp);
    addAutoReleasedObject(manager);
    addAutoReleasedObject(new MakeStepFactory);
    addAutoReleasedObject(new CMakeRunConfigurationFactory);
    addAutoReleasedObject(new CMakeBuildConfigurationFactory);

    addAutoReleasedObject(new CMakeEditorFactory(manager));
    addAutoReleasedObject(new CMakeLocatorFilter);
    addAutoReleasedObject(new CMakeFileCompletionAssistProvider(cmp));

    auto hf = new TextEditor::HighlighterFactory;
    hf->setProductType<CMakeHighlighter>();
    hf->setId(CMakeProjectManager::Constants::CMAKE_EDITOR_ID);
    hf->addMimeType(CMakeProjectManager::Constants::CMAKEMIMETYPE);
    hf->addMimeType(CMakeProjectManager::Constants::CMAKEPROJECTMIMETYPE);
    addAutoReleasedObject(hf);

    return true;
}

void CMakeProjectPlugin::extensionsInitialized()
{
}

Q_EXPORT_PLUGIN(CMakeProjectPlugin)
