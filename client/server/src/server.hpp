#pragma once

class StandardOutLogger;

struct Job : boost::noncopyable
{
    enum JobStatus
    {
        RUNNING_JOB,
        JOB_DONE,
        JOB_ERROR
    };

    JobStatus status;
    const std::string id;
    shared_ptr<Aya::DataModel> dataModel;
    Aya::signals::connection notifyAliveConnection;

    Aya::Time expirationTime;

    Job(const char* id)
        : id(id)
        , status(RUNNING_JOB)
    {
        //
    }

    void touch(double seconds);
    double secondsToTimeout() const;
};

typedef std::map<std::string, boost::shared_ptr<Job>> JobMap;

class Service
{
private:
    JobMap jobs;
    boost::mutex sync;
    long dataModelCount;
    boost::mutex currentlyClosingMutex;

    shared_ptr<Job> createJob(std::string id, std::string name);

public:
    static boost::scoped_ptr<Service> singleton;

    int jobCount() const;

    Service();
    ~Service();

    void closeDataModel(shared_ptr<Aya::DataModel> dataModel);

    void arbiterActivityDump(const std::string& dump); // produces JSON dump of task scheduler avg activity for all jobs
    void diagnosticDump(const std::string& dump);      // produces JSON diagnostic dump

    JobMap::iterator getJob(std::string id);
    void openJob(const std::string& id, const std::string& name, int expirationInSeconds, const std::string& scriptName, const std::string& scriptContent);
    void closeJob(const std::string& id, const char* errorMessage = NULL);
    void closeExpiredJobs(int* result);
    void getAllJobs(const std::string& result); // JSON result of running jobs
    void renderThumbnail(const std::string& type, const std::vector<std::string> arguments, const std::string& result);
    void auditLease(boost::shared_ptr<Job> job);
    void renewLease(const std::string& jobID, double expirationInSeconds);

    void executeScript(const std::string& jobID, const std::string& script, const std::string& result);
    double getExpiration(const std::string& jobID);
};

// clang-format off
void start_aya_server(
    bool ui,
    bool isLAN,
    int gridPort,
    int localServerPort,
    const std::string& localServerPlace,
    const std::string& localServerPassword,
    const std::string& masterServerHost,
    const std::string& masterServerName,
    const std::string& masterServerDescription
);

void stop_aya_server();
bool is_aya_server_running();