//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: doublelabel.h,v 1.2.2.3 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __DOUBLELABEL_H__
#define __DOUBLELABEL_H__

#include <QString>
#include <QSize>
#include <QLabel>
#include <QValidator>

#include "dentry.h"

class QWidget;
class QKeyEvent;

namespace MusEGui {

//---------------------------------------------------------
//   SuperDoubleValidator
//---------------------------------------------------------

class DoubleLabel;
class SuperDoubleValidator : public QValidator {
      Q_OBJECT
  protected:
      DoubleLabel *_dl;
      QString *_cachedText;
      QValidator::State *_cachedState;
      QVariant *_cachedValue;

      QVariant validateAndInterpret(QString &input, int &pos, QValidator::State &state) const;
      QValidator::State validate(QString &input, int &pos) const override;
      void fixup(QString &str) const override;
      void clearCache() const;

  public:
      SuperDoubleValidator(DoubleLabel *parent = nullptr);
      ~SuperDoubleValidator();
      // Strips and trims text of any multiplier character.
      // Returns the multiplier character or null character if none found.
      // Text must already be stripped of any prefix or suffix.
      // The pos is set to the location of the multiplier or -1 if not found.
      QChar findAndStripMultiplier(QString &text, bool doStrip = true, int *pos = nullptr) const;
      // Returns text stripped of any prefix or suffix, and trimmed.
      QString stripped(const QString &text, int *pos = nullptr) const;
};

//---------------------------------------------------------
//   DoubleLabel
//---------------------------------------------------------

class DoubleLabel : public Dentry {
      Q_OBJECT

      Q_PROPERTY( double minValue READ minValue WRITE setMinValue )
      Q_PROPERTY( double maxValue READ maxValue WRITE setMaxValue )
      Q_PROPERTY( double offValue READ off WRITE setOff )
      Q_PROPERTY( QString specialText READ specialText WRITE setSpecialText )
      Q_PROPERTY( QString logZeroSpecialText READ logZeroSpecialText WRITE setLogZeroSpecialText )
      Q_PROPERTY( QString suffix READ suffix WRITE setSuffix )
      Q_PROPERTY( int displayPrecision READ displayPrecision WRITE setDisplayPrecision )
      Q_PROPERTY( bool unlimitedEntryPrecision READ unlimitedEntryPrecision WRITE setUnlimitedEntryPrecision )

      double min, max, dispMin, dispMax, _off, _step, _dBFactor, _dBFactorInv, _logFactor;
      bool _isInteger, _isLog, _isDB, _logCanZero;

      // Text to show if value less than or equal to off value.
      QString _specialText;
      // Text to show if log mode and value is zero (-inf dB).
      QString _logZeroSpecialText;
      QString _suffix;
      int _displayPrecision;
      bool _unlimitedEntryPrecision;
      char _fmt;

      // If it's log, this will return a dB step. Otherwise returns a linear step.
      double calcIncrement() const;
      // Returns true if anything changed.
      virtual bool setNewValue(double v);

      // Returns false if the text failed conversion, true if OK.
      // Changed is set true if the value was changed, false if not.
      virtual bool setSValue(const QString&, bool *changed = nullptr);
      virtual void setString(double val);
      virtual void incValue(int steps = 1);
      virtual void decValue(int steps = 1);

      bool _ignoreCursorPositionChanged;
      SuperDoubleValidator *_validator;

   protected:
      virtual void keyPressEvent(QKeyEvent*);

   protected slots:
      virtual void editorCursorPositionChanged(int oldpos, int newpos);

   public:
      DoubleLabel(QWidget* parent = 0, const char* name = 0);
      DoubleLabel(double val, double min, double max, QWidget* parent = 0,
        bool isLog = false, bool isInteger = false, bool isDB = false);
      virtual QSize sizeHint() const;
      virtual QSize minimumSizeHint () const;
      double minValue() const;
      double maxValue() const;
      // The minimum value converted to the display type (dB etc).
      double displayMinValue() const;
      // The maximum value converted to the display type (dB etc).
      double displayMaxValue() const;
      double off()      const;
      bool isOff() const;
      // In log mode this is a dB step.
      double step()     const;
      void setMinValue(double v);
      void setMaxValue(double v);
      void setRange(double a, double b);
      void setOff(double v);
      // In log mode this is a dB step. In integer mode this is an integer step, even if in log mode.
      void setStep(double v);
      int displayPrecision() const;
      void setDisplayPrecision(int val);
      bool unlimitedEntryPrecision() const;
      // Sets unlimited precision when entering values. It allows entry precision to be different
      //  from display precision, allowing for example entering either 0.000001 OR 1u even with
      //  a display precision of say 3. It also allows entry of precise values even though the
      //  display precision shortens the value shown. The internal value is the precise one entered.
      // This only has an effect if display precision > 0, ie if there is a decimal point at all.
      void setUnlimitedEntryPrecision(bool val);
      // Special 'M' format (Metric suffix G, M, K, m, n, p) supported.
      char textFormat() const;
      // Special 'M' format (Metric suffix G, M, K, m, n, p) supported.
      void setTextFormat(char f);
      QString specialText() const;
      void setSpecialText(const QString& s);
      QString logZeroSpecialText() const;
      void setLogZeroSpecialText(const QString& s);
      QString suffix() const;
      void setSuffix(const QString& s);
      // Sets whether the range represents integers. Can be used with setLog to represent log integers.
      void setInteger(bool v);
      bool isInteger() const;
      // Sets whether the range is logarithmic. Can be used with setInteger to represent log integers.
      void setLog(bool v);
      bool isLog() const;
      // Sets whether the numeric display is in dB when the range is logarithmic.
      void setDisplayDB(bool v);
      bool isDisplayDB() const;
      // Sets whether a log value jumps to zero (-inf dB) when it reaches the given minimum which in most cases
      //  will be set to something above zero (the app's minimum slider dB setting) - even if the controller
      //  itself goes all the way to zero.
      void setLogCanZero(bool v);
      bool logCanZero() const;
      // In log mode, sets the dB factor when conversions are done.
      // For example 20 * log10() for signals, 10 * log10() for power, and 40 * log10() for MIDI volume.
      void setDBFactor(double v = 20.0);
      double dBFactor() const;
      // Sets the scale of a log range. For example a MIDI volume control can set a logFactor = 127
      //  so that the range can conveniently be set to 0-127. (With MIDI volume, dBFactor would be
      //  set to 40.0, as per MMA specs.)
      void setLogFactor(double v = 1.0);
      double logFactor() const;
      double value() const;
      virtual QString textFromValue(double v) const;
      virtual double valueFromText(const QString& s, bool *ok = nullptr) const;
      };


//---------------------------------------------------------
//   DoubleText
//---------------------------------------------------------

class DoubleText : public QLabel {
      Q_OBJECT

      Q_PROPERTY( double minValue READ minValue WRITE setMinValue )
      Q_PROPERTY( double maxValue READ maxValue WRITE setMaxValue )
      Q_PROPERTY( double offValue READ off WRITE setOff )
      Q_PROPERTY( QString specialText READ specialText WRITE setSpecialText )
      Q_PROPERTY( QString logZeroSpecialText READ logZeroSpecialText WRITE setLogZeroSpecialText )
      Q_PROPERTY( QString suffix READ suffix WRITE setSuffix )
      Q_PROPERTY( int precision READ precision WRITE setPrecision )
      Q_PROPERTY( int id READ id WRITE setId )
      Q_PROPERTY( double value READ value WRITE setValue )

      double min, max, _off, _dBFactor, _dBFactorInv, _logFactor;
      bool _isInteger, _isLog, _isDB, _logCanZero;

      // Text to show if value less than or equal to off value.
      QString _specialText;
      // Text to show if log mode and value is zero (-inf dB).
      QString _logZeroSpecialText;
      QString _suffix;
      int _precision;
      char _fmt;

      // Returns true if anything changed.
      virtual bool setNewValue(double v);

      virtual bool setSValue(const QString&);
      virtual void setString(double val);

   protected:
      int _id;
      double val;

   public slots:
      virtual void setValue(double);

   signals:
      void valueChanged(double, int);

   public:
      DoubleText(QWidget* parent = 0, const char* name = 0);
      DoubleText(double val, double min, double max, QWidget* parent = 0,
        bool isLog = false, bool isInteger = false, bool isDB = false);
      virtual QSize sizeHint() const;
      virtual QSize minimumSizeHint () const;
      int id() const;
      void setId(int i);
      double minValue() const;
      double maxValue() const;
      double off()      const;
      void setMinValue(double v);
      void setMaxValue(double v);
      void setRange(double a, double b);
      void setOff(double v);
      int precision() const;
      void setPrecision(int val);
      // Special 'M' format (Metric suffix G, M, K, m, n, p) supported.
      void setTextFormat(char f);
      QString specialText() const;
      void setSpecialText(const QString& s);
      QString logZeroSpecialText() const;
      void setLogZeroSpecialText(const QString& s);
      QString suffix() const;
      void setSuffix(const QString& s);
      // Sets whether the range represents integers. Can be used with setLog to represent log integers.
      void setInteger(bool v);
      // Sets whether the range is logarithmic. Can be used with setInteger to represent log integers.
      void setLog(bool v);
      // Sets whether the numeric display is in dB when the range is logarithmic.
      void setDisplayDB(bool v);
      // Sets whether a log value jumps to zero (-inf dB) when it reaches the given minimum which in most cases
      //  will be set to something above zero (the app's minimum slider dB setting) - even if the controller
      //  itself goes all the way to zero.
      void setLogCanZero(bool v);
      // In log mode, sets the dB factor when conversions are done.
      // For example 20 * log10() for signals, 10 * log10() for power, and 40 * log10() for MIDI volume.
      void setDBFactor(double v = 20.0);
      // Sets the scale of a log range. For example a MIDI volume control can set a logFactor = 127
      //  so that the range can conveniently be set to 0-127. (With MIDI volume, dBFactor would be
      //  set to 40.0, as per MMA specs.)
      void setLogFactor(double v = 1.0);
      double value() const;
      };

} // namespace MusEGui

#endif
