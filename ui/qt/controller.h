/* ui/qt/controller.h --
   Written and Copyright (C) 2017-2019 by vmc.

   This file is part of woinc.

   woinc is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   woinc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with woinc. If not, see <http://www.gnu.org/licenses/>. */

#ifndef WOINC_UI_QT_CONTROLLER_H_
#define WOINC_UI_QT_CONTROLLER_H_

#include <future>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <QObject>
#include <QString>

#include <woinc/types.h>
#include <woinc/ui/defs.h>
#include <woinc/ui/handler.h>

namespace woinc { namespace ui {

struct Controller;

}}

namespace woinc { namespace ui { namespace qt {

struct HandlerAdapter;

class Controller : public QObject {
    Q_OBJECT

    public:
        Controller(QObject *parent = nullptr);
        virtual ~Controller();

        Controller(const Controller&) = delete;
        Controller(Controller &&) = delete;

        Controller &operator=(const Controller&) = delete;
        Controller &operator=(Controller &&) = delete;

    signals:
        void info_occurred(QString title, QString message);
        void warning_occurred(QString title, QString message);
        void error_occurred(QString title, QString message);

    public:
        void register_handler(HostHandler *handler);
        void deregister_handler(HostHandler *handler);

        void register_handler(PeriodicTaskHandler *handler);
        void deregister_handler(PeriodicTaskHandler *handler);

        std::future<GlobalPreferences> load_global_prefs(const QString &host, GET_GLOBAL_PREFS_MODE mode);
        std::future<bool> save_global_prefs(const QString &host,
                                            const GlobalPreferences &prefs,
                                            const GlobalPreferencesMask &mask);
        std::future<bool> read_global_prefs(const QString &host);

        // TODO wording: do we add a host or a client?
        void add_host(QString host, QString url, unsigned short port, QString password);

    public slots:
        void trigger_shutdown();

        void do_active_only_tasks(QString host, bool value);
        void do_file_transfer_op(QString host, FILE_TRANSFER_OP op, QString project_url, QString filename);
        void do_project_op(QString host, QString project_url, PROJECT_OP op);
        void do_task_op(QString host, QString project_url, QString name, TASK_OP op);

        void set_gpu_mode(QString host, RUN_MODE mode);
        void set_network_mode(QString host, RUN_MODE mode);
        void set_run_mode(QString host, RUN_MODE mode);

        void schedule_disk_usage_update(QString host);
        void schedule_projects_update(QString host);
        void schedule_state_update(QString host);
        void schedule_statistics_update(QString host);
        void schedule_tasks_update(QString host);

    public:
        void connect(const HandlerAdapter *adapter) const;

    private slots:
        void handle_host_connected(QString host);
        void handle_host_authorized(QString host);
        void handle_host_authorization_failed(QString host);
        void handle_host_error(QString host, Error error);

    private:
        std::unique_ptr<woinc::ui::Controller> ctrl_;
        std::mutex lock_;
        std::vector<std::pair<QString, QString>> pending_logins_;
};

}}}

#endif
