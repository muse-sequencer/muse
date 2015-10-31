#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2013 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

install( FILES pdf/documentation.pdf pdf/developer_docs.pdf
      DESTINATION ${MusE_DOC_DIR}/muse_pdf
      )

file (GLOB single_documentation_html_files
      html/single/documentation/*.css
      html/single/documentation/*.html
      # html/single/documentation/*.jpg
      html/single/documentation/*.png
      # html/single/documentation/toc_.txt
      )

file (GLOB single_developer_docs_html_files
      html/single/developer_docs/*.css
      html/single/developer_docs/*.html
      # html/single/developer_docs/*.jpg
      html/single/developer_docs/*.png
      # html/single/developer_docs/toc_.txt
      )

file (GLOB split_documentation_html_files
      html/split/documentation/*.css
      html/split/documentation/*.html
      # html/split/documentation/*.jpg
      html/split/documentation/*.png
      # html/split/documentation/toc_.txt
      )

file (GLOB split_developer_docs_html_files
      html/split/developer_docs/*.css
      html/split/developer_docs/*.html
      # html/split/developer_docs/*.jpg
      html/split/developer_docs/*.png
      # html/split/developer_docs/toc_.txt
      )

install( FILES ${single_documentation_html_files}
      DESTINATION ${MusE_DOC_DIR}/muse_html/single/documentation
      )

install( FILES ${single_developer_docs_html_files}
      DESTINATION ${MusE_DOC_DIR}/muse_html/single/developer_docs
      )

install( FILES ${split_documentation_html_files}
      DESTINATION ${MusE_DOC_DIR}/muse_html/split/documentation
      )

install( FILES ${split_developer_docs_html_files}
      DESTINATION ${MusE_DOC_DIR}/muse_html/split/developer_docs
      )
      