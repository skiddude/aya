<script>
    let { page, selectedPage, onClick } = $props();
</script>

<button onclick={onClick} data-page={page.id} title={page.title} class="flex min-h-12 min-w-12 flex-col items-center rounded {page.id == selectedPage ? 'aya-page-btn-selected text-neutral-600 dark:text-gray-200' : 'aya-page-btn text-neutral-500 dark:text-gray-500'}">
    <i class="{page.id == selectedPage ? 'fa-solid' : 'fa-light'} {page.icon} fa-fw"></i>
    <span class="mt-1 text-xs {page.id == selectedPage ? 'font-semibold' : 'font-medium'}">{page.title}</span>
</button>
