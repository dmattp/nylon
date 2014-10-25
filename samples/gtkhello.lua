#! /usr/bin/env lua
--
-- Sample GTK Hello program
--
-- Based on test from LuiGI code.  Thanks Adrian!
--
-- dmp/141024 updated code from LuiGI to demonstrate
-- integration with Nylon.  The use of cords is pretty
-- trivial and pointless, but it demonstrates how Lgi
-- and Nylon can work together.
-- 
-- There is an issue currently that "running" a modal dialog seems
-- to block Nylon events that has to be worked around until fixed.

local lgi = require 'lgi'
local Gtk = lgi.require('Gtk')

require 'site'
local Nylon = require 'nylon.core'()


local cord_about = Nylon.cord('about', function(cord)
   while true do
      cord.event.invoked.wait()
      local dlg = Gtk.AboutDialog {
         program_name = 'LGI Demo',
         title = 'About...',
         name = 'LGI Hello',
         copyright = '(C) Copyright 2010, 2011 Pavel Holejšovský',
         authors = { 'Adrian Perez de Castro', 'Pavel Holejšovský', },
      }
      if tonumber(Gtk._version) >= 3 then
         dlg.license_type = Gtk.License.MIT_X11
      end

      -- dlg:run()
      -- workaround for modal dialogs blocking nylon
      dlg:set_modal(false)
      cord:sleep_manual( function(wakeup)
                            function dlg:on_response(response)
                               wakeup()
                            end
                            dlg:show()
                         end )
      dlg:hide()
   end
end)

Nylon.cord('gtk', function(cord)
   -- Create top level window with some properties and connect its 'destroy'
   -- signal to the event loop termination.
   local window = Gtk.Window {
      title = 'window',
      default_width = 400,
      default_height = 300,
      on_destroy = Gtk.main_quit
   }
   
   if tonumber(Gtk._version) >= 3 then
      window.has_resize_grip = true
   end
   
   -- Create some more widgets for the window.
   local status_bar = Gtk.Statusbar()
   local toolbar = Gtk.Toolbar()
   local ctx = status_bar:get_context_id('default')
   status_bar:push(ctx, 'This is statusbar message.')
   
   -- When clicking at the toolbar 'quit' button, destroy the main window.
   toolbar:insert(Gtk.ToolButton {
   		  stock_id = 'gtk-quit',
   		  on_clicked = function() window:destroy() end,
   	       }, -1)
   
   -- About button in toolbar and its handling.
   local about_button =  Gtk.ToolButton { stock_id = 'gtk-about' }
   function about_button:on_clicked()
      cord_about.event.invoked()
   end
   toolbar:insert(about_button, -1)
   
   -- Pack everything into the window.
   local vbox = Gtk.VBox()
   vbox:pack_start(toolbar, false, false, 0)
   vbox:pack_start(Gtk.Label { label = 'Contents' }, true, true, 0)
   vbox:pack_end(status_bar, false, false, 0)
   window:add(vbox)
   
   -- Show window and start the loop.
   window:show_all()
end) -- end, Gtk cord   

Nylon.cord('printstuff', function(cord)
                            while true do
                               print 'hello from nylon'
                               cord:sleep(1.5)
                            end
                         end)

-- When using Gtk.main, NylonSysCore.initApplication() must be invoked 
-- first to register with glib's event loop.  [This would be otherwise
-- done within Nylon.run()]
NylonSysCore.initApplication()
Gtk.main()
