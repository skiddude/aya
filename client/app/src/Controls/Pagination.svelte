<script>
    import Button from "@/Controls/Button.svelte"

    let { currentPage, lastPage, routeName } = $props();

    const pageNumbers = () => {
        let pages = []
        const maxPages = 10
        let start = Math.max(currentPage - Math.floor(maxPages / 2), 1)
        let end = Math.min(start + maxPages - 1, lastPage)

        if (end - start < maxPages - 1) {
            start = Math.max(end - maxPages + 1, 1)
        }

        for (let i = start; i <= end; i++) {
            pages.push(i)
        }

        return pages
    }

    const goToPage = (pageNumber) => {
        if (pageNumber < 1 || pageNumber > lastPage) return
        window.location.href = window.route(routeName, { page: pageNumber })
    }
</script>

{#if lastPage > 1}
    <div class="mt-4 flex items-center justify-center space-x-0">
        <Button class="aya-btn-sm rounded-l-md rounded-r-none border hover:bg-gray-100 dark:border-gray-600 dark:hover:bg-gray-700" text="‹" disabled={currentPage === 1} onclick={() => goToPage(currentPage - 1)} />
        {#each pageNumbers() as pageNum}
            <Button class={`aya-btn-sm border ${pageNum === currentPage ? "text-gray-700" : "hover:bg-gray-100"} ${pageNum === currentPage ? "cursor-not-allowed" : ""} rounded-none dark:border-gray-600 dark:hover:bg-gray-700`} text={pageNum} disabled={pageNum === currentPage} onclick={() => pageNum !== currentPage && goToPage(pageNum)} />
        {/each}
        <Button class="aya-btn-sm rounded-l-none rounded-r-md border hover:bg-gray-100 dark:border-gray-600 dark:hover:bg-gray-700" text="›" disabled={currentPage === lastPage} onclick={() => goToPage(currentPage + 1)} />
    </div>
{/if}
