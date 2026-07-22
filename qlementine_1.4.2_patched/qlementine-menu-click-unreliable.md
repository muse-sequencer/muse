# Menu item clicks unreliable / can silently fail to trigger (`MenuEventFilter`, v1.4.0+)

## Summary

`MenuEventFilter::eventFilter()` intercepts the real `MouseButtonRelease` on a menu item, runs a
~60ms "flash" highlight animation, and only *after* that completes manually constructs and resends
a synthetic `MouseButtonRelease` via `QCoreApplication::sendEvent()` to actually trigger the action.
In practice this mechanism is unreliable: after the synthetic event is correctly identified and let
through, a second, unexplained `MouseButtonRelease` fires with the filter's internal `_mousePressed`
flag back at `true` - with no matching `MouseButtonPress` logged in between - which re-triggers the
whole flash-and-resend sequence a second time for the same action. The net effect is that menu item
clicks can silently fail to register at all, or behave inconsistently.

## Root cause (as far as I could isolate it)

```cpp
// MenuEventFilter::eventFilter(), case QEvent::Type::MouseButtonRelease:
if (action->menu() == nullptr) {
  flashAction(action, _menu, [this, action]() {
    const auto menuItemCenter = _menu->actionGeometry(action).center();
    _mouseEventToNotFilter.reset(new QMouseEvent(QEvent::MouseButtonRelease, menuItemCenter,
      _menu->mapToGlobal(menuItemCenter), Qt::LeftButton, Qt::NoButton, Qt::NoModifier));
    QCoreApplication::sendEvent(_menu, _mouseEventToNotFilter.get());   // <-- reenters eventFilter
    _mouseEventToNotFilter.reset();
  });
  return true;
}
```

Instrumented with debug logging, a single physical click on a menu item produces this sequence
(trimmed):

```
MouseButtonPress on menu=0x...
MouseButtonPress on menu=0x...                      <- two Press events per one physical click
MouseButtonRelease on menu=0x... _mousePressed=1
starting flashAction for action=Appearance...
flash callback firing, menu=0x... action=Appearance...
sending synthetic release event now
MouseButtonRelease on menu=0x... _mousePressed=0
release is our own resent event, letting it through   <- correctly identified, return false
MouseButtonRelease on menu=0x... _mousePressed=1       <- unexplained second cycle, no Press logged
starting flashAction for action=Appearance...            <- flashAction re-triggered for same action
synthetic release event returned
flash callback firing, menu=0x... action=Appearance...
sending synthetic release event now
MouseButtonRelease on menu=0x... _mousePressed=0
release ignored, _mousePressed was false
synthetic release event returned
```

Two things stood out:
1. Two `MouseButtonPress` events are delivered to the filter for a single physical click, with no
   intervening release.
2. After the synthetic resend is correctly identified (`evt == _mouseEventToNotFilter.get()`) and
   let through (`return false`), a *second* `MouseButtonRelease` arrives with `_mousePressed` back
   at `1`, despite no `MouseButtonPress` having fired in between - re-entering the whole
   flash-and-resend sequence for the same action a second time.

I wasn't able to fully trace *why* the duplicate press/second cycle happens without deeper
interactive debugging, but the underlying issue is clear: reacting to a real click by eating it,
deferring for ~60ms via a timer-driven "flash" animation, and then synthesizing + resending a new
`MouseButtonRelease` through the same object that's still installed as an event filter on itself is
inherently reentrancy-prone, and empirically produces exactly this kind of duplicated/dropped click.

## Fix (what I applied locally)

Bypassed the flash-and-defer-resend mechanism for the actual click-trigger, letting Qt's own stock
`QMenu::mouseReleaseEvent()` handle it directly instead:

```cpp
if (action->menu() == nullptr) {
  return false; // let Qt's own QMenu::mouseReleaseEvent() handle the click directly
}
```

This loses the ~60ms cosmetic highlight-before-trigger effect, but restores reliable, single-fire
click behavior. Confirmed fixed with this change - no more dropped clicks, no more duplicate
flash/trigger cycles.

## Affected versions

- Introduced together with `ComboboxItemViewFilter.hpp` (see my other report) in **v1.4.0** - the
  file doesn't exist at all prior to that release.
- Confirmed present in **v1.4.2** and current **`dev`** (as of this writing) - the delegate-overwrite
  fix that landed nearby (`isDefaultItemDelegate`) doesn't touch this code path.
- **v1.3.0 and earlier are unaffected** (file doesn't exist there), and have the same
  `Theme`/`QlementineStyle` public API I depend on (`fromJsonPath`/`fromJsonDoc`,
  `setTheme`/`theme()`/`themeChanged()`), so downgrading is a viable workaround if you don't want to
  patch.

## Environment

- Qt: 6.x, Linux
- Reproduced in a real Qt Widgets application with several menu bars / context menus; happens on
  ordinary top-level menu items (not submenus - those go through a different code path and aren't
  affected)
