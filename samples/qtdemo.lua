#!/usr/bin/lua

require'qtcore'
require'qtgui'


require 'site'
local Nylon = require 'nylon.core'()

local nyloncord = Nylon.cord('t', function(cord)
              local i = 1
              local sliderValue = 0
              cord.event.currvalue = function(v) sliderValue = v end
              while true do
                 print(i, 'hello from nylon, sliderValue=', sliderValue)
                 i = i + 1
                 cord:sleep(0.5)
              end
end)


Nylon.cord('gui', function(cord)

              local new_MyWidget = function(...)
                 local this = QWidget.new(...)

                 local quit = QPushButton.new('Quit')
                 quit:setFont(QFont.new('Times', 18, 75))

                 local lcd = QLCDNumber.new()
                 lcd:setSegmentStyle'Filled'

                 local slider = QSlider.new'Horizontal'
                 slider:setRange(0, 99)
                 slider:setValue(0)

                 QObject.connect(quit, '2clicked()', function(o)
                                    -- QCoreApplication.instance(), '1quit()'
                                    print('clicked quit')
                                    Nylon.halt()
                                    
                 end )
                 QObject.connect(slider, '2valueChanged(int)', lcd, '1display(int)')
                 QObject.connect(slider, '2valueChanged(int)', function(o,v)
                                    nyloncord.event.currvalue(v)
                 end )

                 local layout = QVBoxLayout.new()
                 layout:addWidget(quit)
                 layout:addWidget(lcd)
                 layout:addWidget(slider)
                 this:setLayout(layout)
                 return this
              end

              widget = new_MyWidget()
              widget:show()
end)


app = QApplication.new(1 + select('#', ...), {arg[0], ...})
app.__gc = app.delete -- take ownership of object

Nylon.runWithCb( function()
                   QCoreApplication.sendPostedEvents()
                   QCoreApplication.processEvents(0)
end )
print 'runWithCb returned'


