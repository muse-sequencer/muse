//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: doublelabel.cpp,v 1.1.1.1.2.2 2008/08/18 00:15:26 terminator356 Exp $
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

#include "muse_math.h"

#include <QWidget>
#include <QKeyEvent>

#include "doublelabel.h"

// For debugging output: Uncomment the fprintf section.
//#include <stdio.h>
#define DEBUG_DOUBLELABEL(dev, format, args...)  // fprintf(dev, format, ##args);

namespace MusEGui {

static QString composeLabelText(double val, char fmt, int prec)
{
  if(fmt == 'M')
  {
    const double av = qAbs(val);
    if(av >= 1.0e9)
      return QString("%L1").arg(val / 1.0e9, 0, 'f', prec) + "G";
    else if(av >= 1.0e6)
      return QString("%L1").arg(val / 1.0e6, 0, 'f', prec) + "M";
    else if(av >= 1.0e3)
      return QString("%L1").arg(val / 1.0e3, 0, 'f', prec) + "K";
    // NOTICE the sequence here: 0.01 will say 0.01, but 0.009 will say 9m.
    // It makes scales symmetrical about 1 and looks more pleasing.
    // For example 0.001 - 1000 = 1m - 1K, and 0.009 - 1000 = 9m - 1K.
    // It also saves a digit!
    // Using 9's to avoid rounding errors.
    else if(av >= 9.9999e-3)
      return QString("%L1").arg(val, 0, 'f', prec);
    else if(av >= 9.9999e-6)
      return QString("%L1").arg(val * 1.0e3, 0, 'f', prec) + "m";
    else if(av >= 9.9999e-9)
      return QString("%L1").arg(val * 1.0e6, 0, 'f', prec) + "u";
    else if(av >= 9.9999e-12)
      return QString("%L1").arg(val * 1.0e9, 0, 'f', prec) + "n";
    else if(av >= 9.9999e-15)
      return QString("%L1").arg(val * 1.0e12, 0, 'f', prec) + "p";
    // Catch zero. And for anything else, just default to scientific format.
    else
      return QString("%L1").arg(val, 0, 'f', prec);
  }
  return QString("%L1").arg(val, 0, fmt, prec);
}

static double multiplierValue(const QChar &c)
{
  if(c == 'G' || c == 'g')
    return 1.0E9;
  if(c == 'M')
    return 1.0E6;
  if(c == 'K' || c == 'k')
    return 1.0E3;
  if(c == 'm')
    return 1.0E-3;
  if(c == 'U' || c == 'u')
    return 1.0E-6;
  if(c == 'N' || c == 'n')
    return 1.0E-9;
  if(c == 'P' || c == 'p')
    return 1.0E-12;
  return 1.0;
}

//---------------------------------------------------------
//   SuperDoubleValidator
//---------------------------------------------------------

SuperDoubleValidator::SuperDoubleValidator(DoubleLabel *parent)
 : QValidator(parent), _dl(parent)
{
  _cachedText = new QString();
  _cachedState = new QValidator::State;
  _cachedValue = new QVariant();
  *_cachedText = QLatin1String("\x01");
  *_cachedState = QValidator::Invalid;
  *_cachedValue = 0.0;
}

SuperDoubleValidator::~SuperDoubleValidator()
{
  if(_cachedText)
    delete _cachedText;
  if(_cachedState)
    delete _cachedState;
  if(_cachedValue)
    delete _cachedValue;
}

// Parameter t must already be stripped of any prefix or suffix.
QChar SuperDoubleValidator::findAndStripMultiplier(QString &t, bool doStrip, int *pos) const
{
  QChar ret;
  int i = -1;
  if((_dl->textFormat() == 'M') &&
     (((i = t.lastIndexOf('G', -1, Qt::CaseInsensitive)) != -1) ||
      ((i = t.lastIndexOf('M', -1, Qt::CaseInsensitive)) != -1) ||
      ((i = t.lastIndexOf('K', -1, Qt::CaseInsensitive)) != -1) ||
      //((i = t.lastIndexOf('m')) != -1) ||
      ((i = t.lastIndexOf('u', -1, Qt::CaseInsensitive)) != -1) ||
      ((i = t.lastIndexOf('n', -1, Qt::CaseInsensitive)) != -1) ||
      ((i = t.lastIndexOf('p', -1, Qt::CaseInsensitive)) != -1)))
  {
    ret = t.at(i);
    if(doStrip)
    {
      t.remove(i, 1);
      t = t.trimmed();
    }
  }
  if(pos)
    *pos = i;
  return ret;
}

QString SuperDoubleValidator::stripped(const QString &t, int *pos) const
{
    QStringRef text(&t);
    if ((_dl->specialText().isEmpty() || text != _dl->specialText()) &&
        (_dl->logZeroSpecialText().isEmpty() || text != _dl->logZeroSpecialText())) {
        int from = 0;
        int size = text.size();
        bool changed = false;
//         if (_dl->prefix().size() && text.startsWith(_dl->prefix())) {
//             from += _dl->prefix().size();
//             size -= from;
//             changed = true;
//         }
        if (!_dl->suffix().isEmpty() && text.endsWith(_dl->suffix())) {
            size -= _dl->suffix().size();
            changed = true;
        }
        if (changed)
            text = text.mid(from, size);
    }
    const int s = text.size();
    text = text.trimmed();
    if (pos)
        (*pos) -= (s - text.size());
    return text.toString();
}

QVariant SuperDoubleValidator::validateAndInterpret(QString &input, int &pos, QValidator::State &state) const
{
    const double min = _dl->displayMinValue();
    const double max = _dl->displayMaxValue();
    const bool unlimEntPrec = _dl->unlimitedEntryPrecision();
    const int prec = _dl->displayPrecision();
    const QLocale loc = locale();
    QString copy = stripped(input, &pos);
    double num = min;
    const bool plus = max >= 0;
    const bool minus = min <= 0;
    const QString group(loc.groupSeparator());
    const uint groupUcs = (group.size() > 1 && group.at(0).isHighSurrogate()
                           ? QChar::surrogateToUcs4(group.at(0), group.at(1))
                           : group.at(0).unicode());

    const bool metricFmt =_dl->textFormat() == 'M';
    bool hasMultiplier = false;
    QChar multiplierChar;
    double multiplierVal = 1.0;
    int multiplierPos = -1;
    if(metricFmt)
    {
      multiplierChar = findAndStripMultiplier(copy, true /*do strip*/, &multiplierPos);
      hasMultiplier = !multiplierChar.isNull();
      multiplierVal = multiplierValue(multiplierChar);
    }

    const int len = copy.size();

    switch (len) {
    case 0:
        state = max != min ? QValidator::Intermediate : QValidator::Invalid;
        goto end;
    case 1:
        if ((prec > 0 && copy.at(0) == loc.decimalPoint())
            || (plus && copy.at(0) == QLatin1Char('+'))
            || (minus && copy.at(0) == QLatin1Char('-'))) {
            state = QValidator::Intermediate;
            goto end;
        }
        break;
    case 2:
        if (prec > 0 && copy.at(1) == loc.decimalPoint()
            && ((plus && copy.at(0) == QLatin1Char('+')) || (minus && copy.at(0) == QLatin1Char('-')))) {
            state = QValidator::Intermediate;
            goto end;
        }
        break;
    default: break;
    }
    if (copy.at(0) == loc.groupSeparator()) {
        state = QValidator::Invalid;
        goto end;
    } else if (len > 1) {
        const int dec = prec > 0 ? copy.indexOf(loc.decimalPoint()) : -1;
        if (dec != -1) {
            if (dec + 1 < copy.size() && copy.at(dec + 1) == loc.decimalPoint() && pos == dec + 1) {
                copy.remove(dec + 1, 1); // typing a delimiter when you are on the delimiter
            } // should be treated as typing right arrow
            // Check if the maximum number of decimals has been typed -
            //  but ignore if unlimited entry precision is enabled.
            if (!unlimEntPrec && (copy.size() - dec > prec + 1)) {
                state = QValidator::Invalid;
                goto end;
            }
            for (int i=dec + 1; i<copy.size(); ++i) {
                if (copy.at(i).isSpace() || copy.at(i) == loc.groupSeparator()) {
                    state = QValidator::Invalid;
                    goto end;
                }
            }
        } else {
            const QChar last = copy.back();
            const bool groupEnd = copy.endsWith(group);
            const QStringView head(copy.constData(), groupEnd ? len - group.size() : len - 1);
            const QChar secondLast = head.back();
            if ((groupEnd || last.isSpace()) && (head.endsWith(group) || secondLast.isSpace())) {
                state = QValidator::Invalid;
                goto end;
            } else if (last.isSpace() && (!QChar::isSpace(groupUcs) || secondLast.isSpace())) {
                state = QValidator::Invalid;
                goto end;
            }
        }
    }
    {
        bool ok = false;
        // No precision? Process as an integer.
        if(prec > 0)
          num = loc.toDouble(copy, &ok);
        else
          num = loc.toInt(copy, &ok);

        if (ok) {
          // Apply the multiplier to the number that was typed in.
          num *= multiplierVal;
        }
        else {
            if (QChar::isPrint(groupUcs)) {
                if (max < 1000 && min > -1000 && copy.contains(group)) {
                    state = QValidator::Invalid;
                    goto end;
                }
                const int len = copy.size();
                for (int i = 0; i < len - 1;) {
                    if (QStringView(copy).mid(i).startsWith(group)) {
                        if (QStringView(copy).mid(i + group.size()).startsWith(group)) {
                            state = QValidator::Invalid;
                            goto end;
                        }
                        i += group.size();
                    } else {
                        i++;
                    }
                }
                QString copy2 = copy;
                copy2.remove(group);
                // No precision? Process as an integer.
                if(prec > 0)
                  num = loc.toDouble(copy2, &ok);
                else
                  num = loc.toInt(copy2, &ok);
                if (ok) {
                  // Apply the multiplier to the number that was typed in.
                  num *= multiplierVal;
                }
                else {
                  state = QValidator::Invalid;
                  goto end;
                }
            }
        }
        if (!ok) {
            state = QValidator::Invalid;
        }
//
// NOTE: Although it is nice to have these stringent bounds checks, they interfere with operation.
//       For example if the min is 0.1000001, that makes it difficult to fixup a 0.1 text because the
//        fixup routine wants to put 0.1 text and we would have to make a special exception in our
//        textFromValue routine to give 0.1000001, which leads us here to the validator needing to accept it.
//       A problem there is textFromValue wants a precision. What precision would be acceptable? 10?, 50? 100?
//       The precision would be arbitrarily large. So it is desirable to avoid that situation.
//
//       So instead, we accept any number entered, and our editingFinished routine simply does the rest.
//       (That is how the previous version of this control worked, it allowed any length number to be entered.)
//       Notice how there is no intermediate state here. Any valid double is accepted.
//
//         else if (num >= min && num <= max) {
//             state = QValidator::Acceptable;
//         } else if (max == min) { // when max and min is the same the only non-Invalid input is max (or min)
//             state = QValidator::Invalid;
//         } else {
//             // Already have a multiplier? Check if the multiplied number might start to be out of range.
//             if ((!metricFmt || hasMultiplier) && ((num >= 0 && num > max) || (num < 0 && num < min))) {
//                   state = QValidator::Invalid;
//             } else {
//                 // For a log value but not dB display, if the number is between zero and min,
//                 //  and log can go to zero, then the number is acceptable.
//                 if(num >= 0.0 && num < min && _dl->isLog() && !_dl->isDisplayDB() && _dl->logCanZero())
//                   state = QValidator::Acceptable;
//                 // Otherwise keep waiting for a possible multiplier, or more digits.
//                 else
//                   state = QValidator::Intermediate;
//             }
//         }
        else {
            state = QValidator::Acceptable;
        }
    }
end:
    if (state != QValidator::Acceptable) {
        num = max > 0 ? min : max;
    }

    if(hasMultiplier)
      input = copy + multiplierChar + _dl->suffix();
    else
      input = copy + _dl->suffix();

    return QVariant(num);
}

void SuperDoubleValidator::clearCache() const
{
    (*_cachedText).clear();
    (*_cachedValue).clear();
    (*_cachedState) = QValidator::Acceptable;
}

QValidator::State SuperDoubleValidator::validate(QString &text, int &pos) const
{
    DEBUG_DOUBLELABEL(stderr, "SuperDoubleValidator::validate: text:%s pos:%d modified:%d\n",
          text.toLatin1().constData(), pos, _dl->isModified());

    if(*_cachedText == text && !text.isEmpty())
      return *_cachedState;

    QValidator::State state;
    const QVariant num = validateAndInterpret(text, pos, state);

    *_cachedText = text;
    *_cachedState = state;
    *_cachedValue = QVariant(num);

    DEBUG_DOUBLELABEL(stderr, "SuperDoubleValidator::validate: validated: text:%s pos:%d state:%d modified:%d\n",
          text.toLatin1().constData(), pos, state, _dl->isModified());

    return state;
}

void SuperDoubleValidator::fixup(QString &input) const
{
  DEBUG_DOUBLELABEL(stderr, "SuperDoubleValidator::fixup: input:%s\n", input.toLatin1().constData());
  input = _dl->textFromValue((_dl->valueFromText(input)));
}

//==============================================================================

//---------------------------------------------------------
//   DoubleLabel
//---------------------------------------------------------

DoubleLabel::DoubleLabel(QWidget* parent, const char* name)
   : Dentry(parent, name), min(0.0), max(1.0), dispMin(min), dispMax(max), _off(min - 10.0),
     _step(0.0), _dBFactor(20.0), _dBFactorInv(1.0/_dBFactor),
     _logFactor(1.0), _isInteger(false), _isLog(false), _isDB(false), _logCanZero(false),
     _specialText("---"), _displayPrecision(3), _unlimitedEntryPrecision(false)
      {
      _validator = new SuperDoubleValidator(this);
      setValidator(_validator);
      _ignoreCursorPositionChanged = false;
      _fmt = 'f'; // Metric suffix G M K m n p.
      setNewValue(0.0);
      connect(this, &Dentry::cursorPositionChanged, [this](int oldpos, int newpos)
        { editorCursorPositionChanged(oldpos, newpos); } );
      }

DoubleLabel::DoubleLabel(double _val, double _min, double _max, QWidget* parent, bool isLog, bool isInteger, bool isDB)
   : Dentry(parent), _step(0.0), _dBFactor(20.0), _dBFactorInv(1.0/_dBFactor), _logFactor(1.0),
      _isInteger(isInteger), _isLog(isLog), _isDB(isDB), _logCanZero(false), _specialText("---"),
      _displayPrecision(3), _unlimitedEntryPrecision(false)
      {
      _validator = new SuperDoubleValidator(this);
      setValidator(_validator);
      _ignoreCursorPositionChanged = false;
      _fmt = 'f'; // Metric suffix G M K m n p.
      setRange(_min, _max);
      setNewValue(_val);
      connect(this, &Dentry::cursorPositionChanged, [this](int oldpos, int newpos)
        { editorCursorPositionChanged(oldpos, newpos); } );
      }

double DoubleLabel::minValue() const               { return min; }
double DoubleLabel::maxValue() const               { return max; }
double DoubleLabel::displayMinValue() const        { return dispMin; }
double DoubleLabel::displayMaxValue() const        { return dispMax; }
double DoubleLabel::off()      const               { return _off; }
bool DoubleLabel::isOff() const                    { return val <= _off && !_specialText.isEmpty(); }
double DoubleLabel::step()     const               { return _step; }
void DoubleLabel::setMinValue(double v)            { setRange(v, max); }
void DoubleLabel::setMaxValue(double v)            { setRange(min, v); }
int DoubleLabel::displayPrecision() const          { return _displayPrecision; }
bool DoubleLabel::unlimitedEntryPrecision() const  { return _unlimitedEntryPrecision; }
QString DoubleLabel::specialText() const           { return _specialText; }
void DoubleLabel::setSpecialText(const QString& s) { _specialText = s; updateGeometry(); setString(val); }
QString DoubleLabel::suffix() const                { return _suffix; }
void DoubleLabel::setSuffix(const QString& s)      { _suffix = s; updateGeometry(); setString(val); }
void DoubleLabel::setLogCanZero(bool v)            { _logCanZero = v; updateGeometry(); }
bool DoubleLabel::logCanZero() const               { return _logCanZero; }
QString DoubleLabel::logZeroSpecialText() const    { return _logZeroSpecialText; }
void DoubleLabel::setLogZeroSpecialText(const QString& s) { _logZeroSpecialText = s; updateGeometry(); setString(val); }
void DoubleLabel::setDBFactor(double v)
{
  // Set the new factor.
  _dBFactor = v;
  _dBFactorInv = 1.0/_dBFactor;
  // Update the range.
  setRange(min, max);
}

double DoubleLabel::dBFactor() const { return _dBFactor; }

void DoubleLabel::setLogFactor(double v)
{
  if(_isLog)
  {
    // Set the new factor.
    _logFactor = v;
    // Update the range.
    setRange(min, max);
    return;
  }

  // Set the new factor.
  _logFactor = v;
  // Update the range.
  setRange(min, max);
}

double DoubleLabel::logFactor() const { return _logFactor; }

void DoubleLabel::setInteger(bool v)
{
  if(v == _isInteger)
    return;

  _isInteger = v;
  // Update the range.
  setRange(min, max);
}

bool DoubleLabel::isInteger() const { return _isInteger; }

void DoubleLabel::setLog(bool v)
{
  if(v == _isLog)
    return;

  // Set the new log.
  _isLog = v;
  // Update the range.
  setRange(min, max);
}

bool DoubleLabel::isLog() const { return _isLog; }

void DoubleLabel::setDisplayDB(bool v)
{
  if(v == _isDB)
    return;
  // Set the new log.
  _isDB = v;
  // Update the range.
  setRange(min, max);
}

bool DoubleLabel::isDisplayDB() const { return _isDB; }

void DoubleLabel::setRange(double a, double b)
{
  double conv_min = a;
  double conv_max = b;

  // If it's integer or log integer.
  if(_isInteger)
  {
    a = rint(a);
    b = rint(b);
  }

  if(_isLog)
  {
    if(_isInteger)
    {
      // Force a hard lower limit of integer 1.
      if(a <= 0.0)
        a = 1.0;
      if(b <= 0.0)
        b = 1.0;
      conv_min = a;
      conv_max = b;
      conv_min /= _logFactor;
      conv_max /= _logFactor;
      conv_min = museValToDb(conv_min, _dBFactor);
      conv_max = museValToDb(conv_max, _dBFactor);
    }
    else if(_isDB)
    {
      // Force a hard lower limit of -120 dB.
      if(a <= 0.0)
      {
        a = 0.000001;
        conv_min = -120;
      }
      else
        conv_min = museValToDb(conv_min, _dBFactor);

      if(conv_max <= 0.0)
      {
        b = 0.000001;
        conv_max = -120;
      }
      else
        conv_max = museValToDb(conv_max, _dBFactor);
    }
  }

  // Unlike a slider for example, there is no 'handedness' here.
  // There is only up and down. Therefore, wholesale swap the min and max
  //  if min is greater than max.
  const double mn = qMin(a, b);
  const double mx = qMax(a, b);
  const double conv_mn = qMin(conv_min, conv_max);
  const double conv_mx = qMax(conv_min, conv_max);

  min = mn;
  max = mx;
  dispMin = conv_mn;
  dispMax = conv_mx;

  _off = min - 10.0;
  updateGeometry();
  // Reset the value.
  setString(val);
}

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void DoubleLabel::setOff(double v)
{
  _off = v;
  setString(val);
  updateGeometry();
}

void DoubleLabel::setStep(double v)
{
  _step = v;
  updateGeometry();
}

//---------------------------------------------------------
//   calcIncrement()
//---------------------------------------------------------

double DoubleLabel::calcIncrement() const
{
  if(_step != 0.0)
    return _step;

  if(_isLog)
    return 0.5;

  // Max is always greater than min.
  const double dif = max - min;
  if(dif <= 10.0)
    return 0.1;
  else
  if(dif <= 100.0)
    return 1.0;
  else
    return 10.0;
}

//---------------------------------------------------------
//   textFromValue
//---------------------------------------------------------

QString DoubleLabel::textFromValue(double v) const
{
  DEBUG_DOUBLELABEL(stderr, "DoubleLabel::textFromValue: v:%f\n", v);

  // Is it the off value or less?
  if(v <= _off)
    return _specialText;

  if(_isLog)
  {
    // Is it outside of the allowable range?
    // NOTE: In this control we don't limit the log lower end to min.
    // This way even a rogue or pre-existing value that is lower than min will
    //  at least be displayed, which is hopefully helpful to inform the user.
    if(v < 0.0 || v > max)
      return QString("---");

    QString s;
    // Special for dB display and log zero: Show a special text.
    if(_isDB && v == 0.0)
    {
      if(_logZeroSpecialText.isEmpty())
        s = QString('-') + QChar(0x221e); // The infinity character
      else
        return _logZeroSpecialText;
    }
    else
    {
      if(_isDB)
        v = museValToDb(v / _logFactor, _dBFactor);
      s = composeLabelText(v, _fmt, _displayPrecision);
    }
    // Attach the suffix.
    if(!_suffix.isEmpty())
      s += _suffix;
    return s;
  }
  else // Not log.
  {
    // Is it outside of the allowable range?
    if(v < min || v > max)
    {
      return QString("---");
    }
    else
    {
      QString s = composeLabelText(v, _fmt, _displayPrecision);
      if(!_suffix.isEmpty())
        s += _suffix;
      return s;
    }
  }
}

//---------------------------------------------------------
//   valueFromText
//---------------------------------------------------------

double DoubleLabel::valueFromText(const QString& s, bool *ok) const
{
  DEBUG_DOUBLELABEL(stderr, "DoubleLabel::valueFromText: s:%s\n", s.toLatin1().constData());

  bool rok;
  // Strip any prefix or suffix.
  QString sTrimmed = _validator->stripped(s);
  // Strip and grab the multiplier character, and get the multiplier value.
  const double multiplier = multiplierValue(_validator->findAndStripMultiplier(sTrimmed));
  double v = sTrimmed.toDouble(&rok);
  if (rok) {
    // Apply the multiplier to the number that was typed in.
    v *= multiplier;

    if(_isLog)
    {
      if(_isDB)
        v = museDbToVal(v, _dBFactorInv) * _logFactor;
      if(v <= min)
      {
        // When value is minimum, is a value of zero allowed?
        if(_logCanZero)
          // Make the value jump to zero.
          v = 0.0;
        else
          v = min;
      }
      if(v > max)
        v = max;
    }
    else // Not log.
    {
      // Limit the range.
      if(v < min)
        v = min;
      if(v > max)
        v = max;
    }

    if(ok)
      *ok = true;
    return v;
  }

  if(ok)
    *ok = false;
  return v;
}

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

void DoubleLabel::setString(double v)
{
  DEBUG_DOUBLELABEL(stderr, "DoubleLabel::setString: v:%f\n", v);
  setText(textFromValue(v));
}

//---------------------------------------------------------
//   setSValue
//---------------------------------------------------------

bool DoubleLabel::setSValue(const QString& s, bool *changed)
{
  DEBUG_DOUBLELABEL(stderr, "DoubleLabel::setSValue: s:%s\n", s.toLatin1().constData());

  bool ok, ret = false, ch = false;
  const double v = valueFromText(s, &ok);

  if(ok)
  {
    DEBUG_DOUBLELABEL(stderr, "DoubleLabel::setSValue: calling setNewValue v:%f\n", v);

    // Set the new value, and emit a signal only if something changed.
    if(setNewValue(v))
    {
      ch = true;
      emit valueChanged(val, _id);
    }
    ret = true;
  }
  if(changed)
    *changed = ch;
  return ret;
}

//---------------------------------------------------------
//   incValue
//---------------------------------------------------------

void DoubleLabel::incValue(int steps)
      {
      if(val >= max)
      {
        val = max;
        return;
      }
      if(val < min)
        val = min;

      double inc = calcIncrement() * double(steps);
      double newval;
      if(_isLog && !_isInteger)
        newval = museDbToVal(museValToDb(val / _logFactor, _dBFactor) + inc, _dBFactorInv) * _logFactor;
      else
        newval = val + inc;
      if(newval >= max)
        newval = max;
      if(setNewValue(newval))
        emit valueChanged(val, _id);
      }

//---------------------------------------------------------
//   decValue
//---------------------------------------------------------

void DoubleLabel::decValue(int steps)
      {
      if(val <= min)
        return;

      double inc = calcIncrement() * double(steps);
      double newval;
      if(_isLog)
      {
        if(_isInteger)
          newval = val - inc;
        else
          newval = museDbToVal(museValToDb(val / _logFactor, _dBFactor) - inc, _dBFactorInv) * _logFactor;
        if(newval <= min)
        {
          if(_logCanZero)
            newval = 0.0;
          else
            newval = min;
        }
      }
      else
      {
        newval = val - inc;
        if(newval <= min)
          newval = min;
      }
      if(setNewValue(newval))
        emit valueChanged(val, _id);
      }

//---------------------------------------------------------
//   setDisplayPrecision
//---------------------------------------------------------

void DoubleLabel::setDisplayPrecision(int v)
      {
      _displayPrecision = v;
      updateGeometry();
      setString(val);
      }

//---------------------------------------------------------
//   setEntryPrecision
//---------------------------------------------------------

void DoubleLabel::setUnlimitedEntryPrecision(bool v)
      {
      _unlimitedEntryPrecision = v;
      }

char DoubleLabel::textFormat() const { return _fmt; }

void DoubleLabel::setTextFormat(char f)
{
    _fmt = f;
    update();
}

void DoubleLabel::keyPressEvent(QKeyEvent* e)
{
  DEBUG_DOUBLELABEL(stderr, "DoubleLabel::keyPressEvent: key:%d modified:%d\n", e->key(), isModified());

  // NOTE: When there is a suffix and there is selected text that includes part or all of the suffix,
  //        this part is required. Because otherwise if the suffix is chopped off by pressing a key,
  //        we get strange results. Our validate() is called TWICE from the one key press, and
  //        unfortunately by the second validate() call, the modified() flag has been RESET.
  //       We rely on that modified() flag in our editingFinished() handler.
  //       Looking through the Qt code, it seems possibly a slight BUG in the way
  //        QWidgetLineControl::finishChange() and QWidgetLineControl::internalSetText()
  //        call each other, resulting in two calls to our validate().
  //       Certainly the modified flag should not be reset like that. I found nothing in our code
  //        that resets it, say by calling setText() which DOES reset the flag. Tim.
  //
  // [ With a suffix (Audio track dB): Press '4'. Good! Modified is true. ]
  //     SuperDoubleValidator::validate: validated: text:4 dB pos:1 state:2 modified:1
  //     SuperDoubleValidator::validate: text:4 pos:1 modified:1
  //   QWidgetLineControl::finishChange
  //   QWidgetLineControl::internalSetText
  //
  // [ But then for some reason we get called again, and the stack looks like this: ]
  //     DoubleLabel::editorTextChanged: text:4 dB
  //     SuperDoubleValidator::validate: validated: text:4 dB pos:1 state:2 modified:0
  //     SuperDoubleValidator::validate: text:4 dB pos:1 modified:0
  //   QWidgetLineControl::finishChange
  //   QWidgetLineControl::internalSetText
  //   QWidgetLineControl::finishChange
  //   QWidgetLineControl::internalSetText
  //
  // [ Full printout with suffix (Audio track dB): ]
  //   Dentry::keyPressEvent: key:52 modified:0
  //   SuperDoubleValidator::validate: text:4 pos:1 modified:1
  //   SuperDoubleValidator::validate: validated: text:4 dB pos:1 state:2 modified:1
  //   SuperDoubleValidator::validate: text:4 dB pos:1 modified:0
  //   SuperDoubleValidator::validate: validated: text:4 dB pos:1 state:2 modified:0
  //   DoubleLabel::editorTextChanged: text:4 dB
  //   Dentry::keyReleaseEvent: key:52 modified:0
  //
  // [ Full printout with no suffix (Midi track): ]
  //   Dentry::keyPressEvent: key:52 modified:0
  //   SuperDoubleValidator::validate: text:4 pos:1 modified:1
  //   SuperDoubleValidator::validate: validated: text:4 pos:1 state:2 modified:1
  //   DoubleLabel::editorTextChanged: text:4
  //   Dentry::keyReleaseEvent: key:52 modified:1

  //=======================================================================================
  // Check which keys would normally cause selected text to be erased (including suffix),
  //  and prevent the suffix from beign erased:
  //=======================================================================================

  // First check common key sequences to leave alone, over specific keys.
  if(e->matches(QKeySequence::Copy) ||
     e->matches(QKeySequence::Cancel) ||
     e->matches(QKeySequence::Undo) ||
     e->matches(QKeySequence::Redo))
  {
    DEBUG_DOUBLELABEL(stderr, "DoubleLabel::keyPressEvent: Ignoring key\n");
    // Let ancestor have it.
    e->ignore();
    Dentry::keyPressEvent(e);
    return;
  }

  // Got selected text and a suffix?
  if(hasSelectedText() && !_suffix.isEmpty())
  {
    // Check text or common key sequences to definitely fix.
    // Note that e->text() will be empty for keys like:
    // Enter, Return, Up, Down, Left, Right, Home, End, PgUp, PgDown etc.
    if(!e->text().isEmpty() ||
        e->matches(QKeySequence::Paste) ||
        e->matches(QKeySequence::Backspace) ||
        e->matches(QKeySequence::Delete) ||
        e->matches(QKeySequence::Cut))
    {
      const int ssz = _suffix.size();
      const int tsz = text().size();
      const int ss = selectionStart();
      const int se = selectionEnd();
      const int suffs = tsz - ssz;
      // Adjust any selection so that any suffix is not erased.
      if(se >= suffs)
      {
        DEBUG_DOUBLELABEL(stderr, "DoubleLabel::keyPressEvent: Fixing selection\n");
        blockSignals(true);
        if(ss >= suffs)
          deselect();
        else
          setSelection(ss, suffs - ss);
        blockSignals(false);
      }
    }
  }

  e->ignore();
  Dentry::keyPressEvent(e);
  return;
}

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize DoubleLabel::sizeHint() const
      {
      QFontMetrics fm = fontMetrics();
      int h           = fm.height() + 9;
      int n = _displayPrecision;

      ++n;  // For some reason I have to add one digit. Shouldn't have to.
      double aval = qMax(qAbs(max), qAbs(min));
      if (aval >= 10.0)
            ++n;
      if (aval >= 100.0)
            ++n;
      if (aval >= 1000.0)
            ++n;
      if (aval >= 10000.0)
            ++n;
      if (aval >= 100000.0)
            ++n;

// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int w = fm.horizontalAdvance(QString("-0.")) + fm.horizontalAdvance('0') * n + 6;
#else
      int w = fm.width(QString("-0.")) + fm.width('0') * n + 6;
#endif
      if(!_suffix.isEmpty())
      {
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
        w += fm.horizontalAdvance(QString(" ")) + fm.horizontalAdvance(_suffix);
#else
        w += fm.width(QString(" ")) + fm.width(_suffix);
#endif
      }
      return QSize(w, h);
      }

QSize DoubleLabel::minimumSizeHint() const
{
  return sizeHint();
}

bool DoubleLabel::setNewValue(double v)
{
      if (v == val)
        return false;
      val = v;
      setString(v);
      return true;
}

double DoubleLabel::value() const
{
  return val;
}

void DoubleLabel::editorCursorPositionChanged(int oldpos, int newpos)
{
    if (!hasSelectedText() && !_ignoreCursorPositionChanged && !isOff()) {
        _ignoreCursorPositionChanged = true;
        bool allowSelection = true;
        int pos = -1;
//         if (newpos < prefix.size() && newpos != 0) {
//             if (oldpos == 0) {
//                 allowSelection = false;
//                 pos = prefix.size();
//             } else {
//                 pos = oldpos;
//             }
//         }
//         else
        if (newpos > text().size() - _suffix.size()
                   && newpos != text().size()) {
            if (oldpos == text().size()) {
                pos = text().size() - _suffix.size();
                allowSelection = false;
            } else {
                pos = text().size();
            }
        }
        if (pos != -1) {
            const int selSize = selectionStart() >= 0 && allowSelection
                                  ? (selectedText().size()
                                     * (newpos < pos ? -1 : 1)) - newpos + pos
                                  : 0;
            const QSignalBlocker blocker(this);
            if (selSize != 0) {
                setSelection(pos - selSize, selSize);
            } else {
                setCursorPosition(pos);
            }
        }
        _ignoreCursorPositionChanged = false;
    }
}


//=========================================================================================


//---------------------------------------------------------
//   DoubleText
//---------------------------------------------------------

DoubleText::DoubleText(QWidget* parent, const char* name)
   : QLabel(parent), min(0.0), max(1.0), _off(min - 10.0), _dBFactor(20.0), _dBFactorInv(1.0/_dBFactor),
     _logFactor(1.0), _isInteger(false), _isLog(false), _isDB(false), _logCanZero(false),
     _specialText("---"), _precision(3), _id(0), val(0.0)
      {
      setObjectName(name);
      _fmt = 'f'; // Metric suffix G M K m n p.
      setNewValue(0.0);
      }

DoubleText::DoubleText(double _val, double _min, double _max, QWidget* parent, bool isLog, bool isInteger, bool isDB)
   : QLabel(parent), _dBFactor(20.0), _dBFactorInv(1.0/_dBFactor), _logFactor(1.0),
      _isInteger(isInteger), _isLog(isLog), _isDB(isDB), _logCanZero(false),
      _specialText("---"), _precision(3), _id(0), val(0.0)
      {
      _fmt = 'f'; // Metric suffix G M K m n p.
      setRange(_min, _max);
      setNewValue(_val);
      }

int DoubleText::id() const                        { return _id; }
void DoubleText::setId(int i)                     { _id = i; }
double DoubleText::minValue() const               { return min; }
double DoubleText::maxValue() const               { return max; }
double DoubleText::off()      const               { return _off; }
void DoubleText::setMinValue(double v)            { min = v; updateGeometry(); }
void DoubleText::setMaxValue(double v)            { max = v; updateGeometry(); }
int DoubleText::precision() const                 { return _precision; }
QString DoubleText::specialText() const           { return _specialText; }
void DoubleText::setSpecialText(const QString& s) { _specialText = s; updateGeometry(); setString(val); }
QString DoubleText::suffix() const                { return _suffix; }
void DoubleText::setSuffix(const QString& s)      { _suffix = s; updateGeometry(); setString(val); }
void DoubleText::setLogCanZero(bool v)            { _logCanZero = v; updateGeometry(); }
QString DoubleText::logZeroSpecialText() const    { return _logZeroSpecialText; }
void DoubleText::setLogZeroSpecialText(const QString& s) { _logZeroSpecialText = s; updateGeometry(); setString(val); }
void DoubleText::setDBFactor(double v)
{
  // Set the new factor.
  _dBFactor = v;
  _dBFactorInv = 1.0/_dBFactor;
  // Update the geometry.
  updateGeometry();
  // Reset the value.
  setValue(val);
}

void DoubleText::setLogFactor(double v)
{
  if(_isLog)
  {
    // Set the new factor.
    _logFactor = v;
    updateGeometry();
    // Reset the value.
    setValue(val);
    return;
  }

  // Set the new factor.
  _logFactor = v;
  updateGeometry();
}

void DoubleText::setInteger(bool v)
{
  _isInteger = v;
  updateGeometry();
}

void DoubleText::setLog(bool v)
{
  if(v == _isLog)
    return;

  // Set the new log.
  _isLog = v;
  updateGeometry();
  // Reset the value.
  setValue(val);
}

void DoubleText::setDisplayDB(bool v)
{
  if(v == _isDB)
    return;
  // Set the new log.
  _isDB = v;
  updateGeometry();
  // Reset the value.
  setValue(val);
}

void DoubleText::setRange(double a, double b)
{
  // Unlike a slider for example, there is no 'handedness' here.
  // There is only up and down. Therefore, wholesale swap the min and max
  //  if min is greater than max.
  min = qMin(a, b);
  max = qMax(a, b);

  // Force a hard lower limit of -120 dB.
  if(_isLog)
  {
    if(min <= 0.0)
      min = 0.000001 * _logFactor;
    if(max <= 0.0)
      max = 0.000001 * _logFactor;
  }
  _off = min - 10.0;
  updateGeometry();
}

//---------------------------------------------------------
//   setOff
//---------------------------------------------------------

void DoubleText::setOff(double v)
{
  _off = v;
  setString(val);
  updateGeometry();
}

//---------------------------------------------------------
//   setString
//---------------------------------------------------------

void DoubleText::setString(double v)
      {
      // Is it the off value or less?
      if(v <= _off)
      {
        setText(_specialText);
        return;
      }

      if(_isLog)
      {
        // Is it outside of the allowable range?
        // NOTE: In this control we don't limit the log lower end to min.
        // This way even a rogue or pre-existing value that is lower than min
        //  will at least be displayed, which is helpful to inform the user.
        if(v < 0.0 || v > max)
        {
          setText(QString("---"));
          return;
        }

        QString s;

        // Special for dB display and log zero: Show a special text.
        if(_isDB && v == 0.0)
        {
          if(_logZeroSpecialText.isEmpty())
            s = QString('-') + QChar(0x221e); // The infinity character
          else
          {
            setText(_logZeroSpecialText);
            return;
          }
        }
        else
        {
          if(_isDB)
            v = museValToDb(v / _logFactor, _dBFactor);
          s = composeLabelText(v, _fmt, _precision);
        }

        // Attach the suffix.
        if(!_suffix.isEmpty())
        {
          s += " ";
          s += _suffix;
        }
        setText(s);
      }
      else // Not log.
      {
        // Is it outside of the allowable range?
        if(v < min || v > max)
        {
          setText(QString("---"));
          return;
        }
        else
        {
          QString s;
          s = composeLabelText(v, _fmt, _precision);
          if (!_suffix.isEmpty()) {
                s += " ";
                s += _suffix;
                }

          setText(s);
        }
      }
      }

//---------------------------------------------------------
//   setSValue
//---------------------------------------------------------

bool DoubleText::setSValue(const QString& s)
{
  bool ok;
  QString sTrimmed = s.trimmed();
  if (sTrimmed.contains(_suffix))
      sTrimmed = sTrimmed.remove(_suffix).trimmed();
  double v = sTrimmed.toDouble(&ok);
  if (ok) {
    if(_isLog)
    {
      if(_isDB)
        v = museDbToVal(v, _dBFactorInv) * _logFactor;
      if(v <= min)
      {
        // When value is minimum, is a value of zero allowed?
        if(_logCanZero)
          // Make the value jump to zero.
          v = 0.0;
        else
          v = min;
      }
      if(v > max)
        v = max;
    }
    else // Not log.
    {
      // Limit the range.
      if(v < min)
        v = min;
      if(v > max)
        v = max;
    }

    // Set the new value, and emit a signal only if something changed.
    if(setNewValue(v))
    {
      emit valueChanged(val, _id);
      return true;
    }
  }
  return false;
}

//---------------------------------------------------------
//   setPrecision
//---------------------------------------------------------

void DoubleText::setPrecision(int v)
      {
      _precision = v;
      updateGeometry();
      setString(val);
      }

void DoubleText::setTextFormat(char f)
{
    _fmt = f;
    update();
}

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize DoubleText::sizeHint() const
      {
      QFontMetrics fm = fontMetrics();
      int h           = fm.height() + 9;
      int n = _precision;

      ++n;  // For some reason I have to add one digit. Shouldn't have to.
      double aval = qMax(qAbs(max), qAbs(min));
      if (aval >= 10.0)
            ++n;
      if (aval >= 100.0)
            ++n;
      if (aval >= 1000.0)
            ++n;
      if (aval >= 10000.0)
            ++n;
      if (aval >= 100000.0)
            ++n;

// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int w = fm.horizontalAdvance(QString("-0.")) + fm.horizontalAdvance('0') * n + 6;
#else
      int w = fm.width(QString("-0.")) + fm.width('0') * n + 6;
#endif
      if(!_suffix.isEmpty())
      {
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
        w += fm.horizontalAdvance(QString(" ")) + fm.horizontalAdvance(_suffix);
#else
        w += fm.width(QString(" ")) + fm.width(_suffix);
#endif
      }
      return QSize(w, h);
      }

QSize DoubleText::minimumSizeHint() const
{
  return sizeHint();
}

bool DoubleText::setNewValue(double v)
{
      if (v == val)
        return false;
      val = v;
      setString(v);
      return true;
}

double DoubleText::value() const
{
  return val;
}

void DoubleText::setValue(double v)
{
  setNewValue(v);
}

} // namespace MusEGui
