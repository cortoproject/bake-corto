#include <include/corto.h>

static
void gen_source(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project,
    char *source,
    char *target)
{
    bool c4cpp = driver->get_attr_bool("c4cpp");

    ut_strbuf cmd = UT_STRBUF_INIT;
    char *scope = project->id;
    char *scope_attr = driver->get_attr_string("scope");
    char *model = driver->get_attr_string("model");

    if (strlen(scope_attr)) {
        scope = scope_attr;
    }

    if (model) {
        ut_strbuf_append(
            &cmd,
            "corto pp project.json %s --path %s --scope %s --lang %s",
            model,
            project->path,
            scope,
            project->language);
    } else {
        ut_strbuf_append(
            &cmd,
            "corto pp -g c/interface -g c/project");
    }

    ut_strbuf_append(
        &cmd,
        " --name %s --attr c=src --attr cpp=src --attr h=include --attr hpp=include --attr hidden=.bake_cache/gen",
        project->id);

    if (!project->public) {
        ut_strbuf_append(&cmd, " --attr local=true");
    }

    if (project->type != BAKE_PACKAGE) {
        ut_strbuf_append(&cmd, " --attr app=true");
    }

    if (c4cpp) {
        ut_strbuf_append(&cmd, " --c4cpp");
    }

    if (ut_ll_count(project->use)) {
        ut_strbuf imports = UT_STRBUF_INIT;
        ut_strbuf_append(&imports, " --use ");
        ut_iter it = ut_ll_iter(project->use);
        int count = 0;
        while (ut_iter_hasNext(&it)) {
            char *use = ut_iter_next(&it);
            if (!strcmp(use, strarg("%s/c", project->id)) ||
                !strcmp(use, strarg("%s/cpp", project->id)))
            {
                /* Should not add own generated language packages because they
                 * may not yet exist */
                continue;
            }

            if (count) {
                ut_strbuf_append(&imports, ",");
            }
            ut_strbuf_appendstr(&imports, use);
            count ++;
        }
        if (count) {
            char *importStr = ut_strbuf_get(&imports);
            ut_strbuf_appendstr(&cmd, importStr);
        }
    }

    if (ut_ll_count(project->use_private)) {
        ut_strbuf imports = UT_STRBUF_INIT;
        ut_strbuf_append(&imports, " --use-private ");
        ut_iter it = ut_ll_iter(project->use_private);
        int count = 0;
        while (ut_iter_hasNext(&it)) {
            char *use = ut_iter_next(&it);
            if (!strcmp(use, strarg("%s/c", project->id)) ||
                !strcmp(use, strarg("%s/cpp", project->id)))
            {
                /* Should not add own generated language packages because
                 * they may not yet exist */
                continue;
            }
            if (count) {
                ut_strbuf_append(&imports, ",");
            }
            ut_strbuf_appendstr(&imports, use);
            count ++;
        }
        if (count) {
            char *importStr = ut_strbuf_get(&imports);
            ut_strbuf_appendstr(&cmd, importStr);
        }
    }

    char *cmdstr = ut_strbuf_get(&cmd);
    driver->exec(cmdstr);
    free(cmdstr);
}

static
void build_generated(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project)
{
    char *c_api = ut_asprintf("%s/c", project->path);
    if (ut_file_test(c_api) == 1 && ut_isdir(c_api)) {
        char *cmd = ut_asprintf("bake %s", c_api);
        driver->exec(cmd);
        free(cmd);
    }

    char *cpp_api = ut_asprintf("%s/cpp", project->path);
    if (ut_file_test(cpp_api) == 1 && ut_isdir(cpp_api)) {
        char *cmd = ut_asprintf("bake %s", cpp_api);
        driver->exec(cmd);
        free(cmd);
    }

    if (driver->get_attr_bool("use-generated-api") && project->public) {
        char *c_api_id = ut_asprintf("%s.c", project->id);
        driver->use(c_api_id);
        free(c_api_id);

        if (!strcmp(project->language, "cpp")) {
            char *cpp_api_id = ut_asprintf("%s.cpp", project->id);
            driver->use(cpp_api_id);
            free(cpp_api_id);
        }
    }

    free(c_api);
    free(cpp_api);
}

static
char* find_model(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *p)
{
    char *model = NULL;
    ut_iter it;
    if (ut_dir_iter(p->path, "model.*", &it)) {
        p->error = true;
        goto error;
    }

    if (ut_iter_hasNext(&it)) {
        model = ut_iter_next(&it);
        driver->set_attr_string("model", model);
    }

    ut_iter_release(&it);

    return driver->get_attr_string("model");
error:
    return NULL;
}

static
void init(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project)
{
    driver->use("corto");

    if (!driver->get_attr("use-generated-api")) {
        driver->set_attr_bool("use-generated-api", true);
    }

    if (project->language && !strcmp(project->language, "cpp")) {
        driver->set_attr_bool("c4cpp", true);
    }

    ut_ll_append(project->use_build, ut_strdup("driver.gen.c.project"));
    ut_ll_append(project->use_build, ut_strdup("driver.gen.c.interface"));

    char *model = find_model(driver, config, project);
    if (model) {
        ut_ll_append(project->use_build, ut_strdup("driver.gen.c.type"));
        ut_ll_append(project->use_build, ut_strdup("driver.gen.c.api"));
        ut_ll_append(project->use_build, ut_strdup("driver.gen.c.cpp"));
    }

    uint32_t i = 0, count = ut_ll_count(project->use);

    /* Automatically add language-specific extensions */
    if (driver->get_attr_bool("use-generated-api")) {
        ut_iter it = ut_ll_iter(project->use);
        while (ut_iter_hasNext(&it)) {
            const char *dep = ut_iter_next(&it);
            char *apidep = ut_asprintf("%s.%s", dep, project->language);
            if (driver->exists(apidep)) {
                driver->use(apidep);
            } else {
                free(apidep);
            }

            i ++;
            if (i == count) {
                break;
            }
        }
    }
}

static
void clean(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *p)
{
    driver->remove("include/_type.h");
    driver->remove("include/_load.h");
    driver->remove("include/_interface.h");
    driver->remove("include/_api.h");
    driver->remove("include/_cpp.h");
    driver->remove("include/_binding.h");
    driver->remove("c");
    driver->remove("cpp");
}

int bakemain(bake_driver_api *driver) {

    /* Callback that initializes projects with the right build dependencies */
    driver->init(init);

    /* Callback that specifies files to clean */
    driver->clean(clean);

    /* Callback that builds generated projects before building the project */
    driver->prebuild(build_generated);

    /* Add project paths to be ignored by bake discovery */
    driver->ignore_path("c");
    driver->ignore_path("cpp");

    /* Pattern that matches generated files */
    driver->pattern("gen-src", ".bake_cache/gen//*.c|*.cpp|*.cxx");

    /* Specify input and output for generated files */
    driver->rule("GENERATED-SOURCES", "model.*,project.json",
        driver->target_pattern("$gen-src"), gen_source);

    return 0;
}
