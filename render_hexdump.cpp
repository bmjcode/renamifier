/*
 * Hex dump renderer, used as a fallback for unknown file formats.
 * Copyright (c) 2021-2026 Benjamin Johnson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cctype>

#include <QtCore>

#include "render_hexdump.h"

// Stop reading after this many bytes to avoid running out of memory
#define MAX_FILE_SIZE 1048576   // 1 MiB

static const QString hexDump(QIODevice &device, qint64 limit = 0);

HexDumpRenderer::HexDumpRenderer()
    : TextContentRenderer()
{
}

bool HexDumpRenderer::load()
{
    return true;
}

void HexDumpRenderer::render()
{
    QFile file(path());
    QString output = hexDump(file, MAX_FILE_SIZE);
    if (output.isEmpty()) {
        emit errorEncountered(file.errorString());
        return;
    } else
        emit renderedText(output);
}

/*
 * Returns a hex dump of the specified file in the style of xxd(1).
 */
const QString hexDump(QIODevice &device, qint64 limit)
{
    QString output;
    QTextStream outputStream(&output);
    outputStream.setPadChar('0');

    outputStream << Qt::hex;
    if (device.open(QIODevice::ReadOnly)) {
        char c[16];
        int bytesRead, position;

        position = 0;
        while (!device.atEnd()) {
            // Output the stream position
            outputStream.setFieldWidth(8);
            outputStream << position;
            outputStream.setFieldWidth(0);
            outputStream << ": ";

            // Read a line of up to 16 bytes
            bytesRead = device.read(c, 16);
            position += bytesRead;

            // Output the hex dump in two-byte columns
            for (int i = 0; i < 16; ++i) {
                outputStream.setFieldWidth(2);
                if (i < bytesRead)
                    outputStream << (uint8_t)c[i];
                else
                    outputStream << "  ";  // pad lines shorter than 16 chars

                // Add one space between columns, and two spaces between the
                // final column and the human-readable version
                outputStream.setFieldWidth(0);
                if ((i + 1) % 2 == 0)
                    outputStream << " ";
                if ((i + 1) % 16 == 0)
                    outputStream << " ";
            }

            // Output a human-readable version containing all the printable
            // characters in this line
            for (int i = 0; i < std::min(bytesRead, 16); ++i)
                outputStream << (std::isprint(c[i]) ? c[i] : '.');
            outputStream << Qt::endl;

            if (limit != 0 && position >= limit) {
                if (!device.atEnd())
                    outputStream << Qt::dec
                                 << "Remaining "
                                 << device.size() - position
                                 << " bytes omitted.";
                break;
            }
        }
        device.close();
    } else
        output = device.errorString();
    return output;
}
